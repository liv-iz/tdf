/* Thank you to https://editor.p5js.org/ima_ml/sketches/8xB4wpH16 and https://editor.p5js.org/loopstick/sketches/MWZxoSNoP which were used as a basis for this code. Thank you to Gemini for helping with debugging and reducing noise by only sending data when value changes.*/


       // Serial Communication
        let pHtmlMsg;
        let serialOptions = { baudRate: 115200 };
        let serial;
        let MtrSpd = 0;
        let lastSentSpeed = -1;

        // ML5 Face Tracking
        let video;
        let faceMesh;
        let faces = [];

        // Lip Landmark Arrays for Drawing 
        const lipsExterior = [267, 269, 270, 409, 291, 375, 321, 405, 314, 17, 84, 181, 91, 146, 61, 185, 40, 39, 37, 0];
        const lipsInterior = [13, 312, 311, 310, 415, 308, 324, 318, 402, 317, 14, 87, 178, 88, 95, 78, 191, 80, 81, 82];

        // --- Mouth Control Logic Variables ---
        const MOUTH_OPEN_THRESHOLD = 2;
        const MEASUREMENT_INTERVAL = 1000;
        let mouthOpenings = 0;
        let isMouthOpen = false;

        function preload() {
            // Load faceMesh model with refineLandmarks for more accurate lip points.
            faceMesh = ml5.faceMesh({ refineLandmarks: true });
        }

        function setup() {
            createCanvas(640, 480);

            video = createCapture(VIDEO);
            video.size(width, height);
            video.hide();

            // Start detection loop.
            faceMesh.detectStart(video, gotFaces);

            // Reverse the interior array for correct drawing with beginContour.
            lipsInterior.reverse();

            // Setup web serial
            serial = new Serial();
            serial.on(SerialEvents.CONNECTION_OPENED, onSerialConnectionOpened);
            serial.on(SerialEvents.CONNECTION_CLOSED, onSerialConnectionClosed);
            serial.on(SerialEvents.DATA_RECEIVED, onSerialDataReceived);
            serial.on(SerialEvents.ERROR_OCCURRED, onSerialErrorOccurred);

            serial.autoConnectAndOpenPreviouslyApprovedPort(serialOptions);

            pHtmlMsg = createP("Click the canvas to open the serial connection dialog");
            pHtmlMsg.style("color", "#00000");

            // Set an interval to run the speed calculation once per second
            setInterval(calculateAndSendSpeed, MEASUREMENT_INTERVAL);
        }

        // Callback function for when ml5.js finds faces
        function gotFaces(newFaces) {
            faces = newFaces;
        }

        function draw() {
            // Mirror video feed
            translate(width, 0);
            scale(-1, 1);
            image(video, 0, 0, width, height);

            if (faces.length > 0) {
                const keypoints = faces[0].keypoints;

                // Mouth Logic
                // logic relies on distance between inner lip points.
                const upperLip = keypoints[13];
                const lowerLip = keypoints[14];

                if (upperLip && lowerLip) {
                    const mouthOpeningDistance = dist(
                        upperLip.x,
                        upperLip.y,
                        lowerLip.x,
                        lowerLip.y
                    );

                    // Check for the moment the mouth transitions from closed to open.
                    if (mouthOpeningDistance > MOUTH_OPEN_THRESHOLD && !isMouthOpen) {
                        isMouthOpen = true;
                        mouthOpenings++; // Increment the counter for this interval
                    } else if (mouthOpeningDistance < MOUTH_OPEN_THRESHOLD && isMouthOpen) {
                        isMouthOpen = false; // Reset state when mouth closes.
                    }
                }


            
                // Draw the detailed lip shape.
                noStroke();
                fill(255, 0, 255, 150);

                beginShape();
                for (let i of lipsExterior) {
                    let p = keypoints[i];
                    if (p) vertex(p.x, p.y);
                }

                beginContour();
                for (let i of lipsInterior) {
                    let p = keypoints[i];
                    if (p) vertex(p.x, p.y);
                }
                endContour();
                endShape(CLOSE);

                //Text Display
                // Flip back to draw text correctly without mirroring it.
                translate(width, 0);
                scale(-1, 1);

                fill(255);
                stroke(0);
                strokeWeight(4);
                textSize(24);
                textAlign(LEFT, TOP);
                // Display the motor speed
                text(`Motor Speed: ${MtrSpd}`, 10, 10);            }
        }

        // Function runs every MEASUREMENT_INTERVAL
        function calculateAndSendSpeed() {
            if (mouthOpenings === 0) {
                // If there's no movement, the motor speed is 0.
                MtrSpd = 0;
            } else {
                // If there is movement (1 to 6+ times/sec), map to 85-255 range.
                MtrSpd = map(mouthOpenings, 1, 6, 85, 255);
            }

            // Ensure the speed is within the valid 0-255 range (done with Gemini)
            MtrSpd = constrain(MtrSpd, 0, 255);
            MtrSpd = int(MtrSpd);

            // Only send the data if the speed has changed and the port is open (Done with Gemini)
            if (serial.isOpen() && MtrSpd !== lastSentSpeed) {
                console.log(`Sending speed for ${mouthOpenings} openings/sec:`, MtrSpd);
                serial.writeLine(MtrSpd);
                lastSentSpeed = MtrSpd;
            }

            // Reset the counter for next interval
            mouthOpenings = 0;
        }

        // Web Serial Helper functions below (Sudhu's tutorial)

        function onSerialErrorOccurred(eventSender, error) {
            console.log("onSerialErrorOccurred", error);
            pHtmlMsg.html(error);
        }

        function onSerialConnectionOpened(eventSender) {
            console.log("onSerialConnectionOpened");
            pHtmlMsg.html("Serial connection opened successfully!");
        }

        function onSerialConnectionClosed(eventSender) {
            console.log("onSerialConnectionClosed");
            pHtmlMsg.html("Serial connection closed");
        }

        function onSerialDataReceived(eventSender, newData) {
            console.log("onSerialDataReceived", newData);
            pHtmlMsg.html("Received: " + newData);
        }

        function mouseClicked() {
            // Check if the click is within the canvas bounds
            if (mouseX > 0 && mouseX < width && mouseY > 0 && mouseY < height) {
                if (!serial.isOpen()) {
                    serial.connectAndOpen(null, serialOptions);
                }
            }
        }