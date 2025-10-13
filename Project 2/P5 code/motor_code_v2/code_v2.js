
      /* This is an example where I used gemini to change the logic!! */
      
      // --- Serial Communication & Motor Variables ---
      let pHtmlMsg;
      let serialOptions = { baudRate: 115200 };
      let serial;
      let MtrSpd = 0;
      let lastSentSpeed = -1;

      // --- ML5 Face Tracking & Video Variables ---
      let video;
      let faceMesh;
      let faces = [];

      // --- Mouth Control Logic Variables ---
      const MOUTH_OPEN_THRESHOLD = 15; // Sensitivity for mouth opening. Adjust as needed.
      let openingTimestamps = []; // Stores timestamps of when the mouth opens.
      let isMouthOpen = false; // State variable to detect a single open event transition.

      function preload() {
        // Load the faceMesh model
        faceMesh = ml5.faceMesh();
      }

      function setup() {
        createCanvas(640, 480);

        video = createCapture(VIDEO);
        video.size(width, height);
        video.hide();

        // Start the first face detection
        faceMesh.detect(video, gotFaces);

        // --- Setup Web Serial using serial.js ---
        serial = new Serial();
        serial.on(SerialEvents.CONNECTION_OPENED, onSerialConnectionOpened);
        serial.on(SerialEvents.CONNECTION_CLOSED, onSerialConnectionClosed);
        serial.on(SerialEvents.DATA_RECEIVED, onSerialDataReceived);
        serial.on(SerialEvents.ERROR_OCCURRED, onSerialErrorOccurred);

        // Attempt to auto-connect to a previously approved port
        serial.autoConnectAndOpenPreviouslyApprovedPort(serialOptions);

        // Add a <p> element for status messages
        pHtmlMsg = createP(
          "Click anywhere on this page to open the serial connection dialog"
        );
        pHtmlMsg.style("color", "deeppink");
      }

      // Callback for when ml5.js finds faces
      function gotFaces(newFaces) {
        faces = newFaces;
        // Re-run the detection immediately to create a continuous loop
        faceMesh.detect(video, gotFaces);
      }

      function draw() {
        // Mirror the video feed for a more natural feel
        translate(width, 0);
        scale(-1, 1);
        image(video, 0, 0, width, height);
        
        // --- Inactivity Check ---
        // If there's a history of openings and the last one was over 3 seconds ago, stop the motor.
        if (openingTimestamps.length > 0) {
          const timeSinceLastOpen = millis() - openingTimestamps[openingTimestamps.length - 1];
          if (timeSinceLastOpen > 3000) {
            MtrSpd = 0; // Stop the motor
            sendSpeedIfChanged();
            openingTimestamps = []; // Reset history after the pause
          }
        }

        if (faces.length > 0) {
          const keypoints = faces[0].keypoints;

          // Landmarks for the eyes to calculate inter-ocular distance (iod) for scaling visuals
          const leftEye = keypoints[33]; // Left eye inner corner
          const rightEye = keypoints[263]; // Right eye inner corner
          const iod = dist(leftEye.x, leftEye.y, rightEye.x, rightEye.y);

          // Landmarks for the upper and lower inner lip
          const upperLip = keypoints[13];
          const lowerLip = keypoints[14];
          const mouthOpeningDistance = dist(
            upperLip.x,
            upperLip.y,
            lowerLip.x,
            lowerLip.y
          );

          // --- Event Detection Logic ---
          // This checks for the moment the mouth *transitions* from closed to open.
          if (mouthOpeningDistance > MOUTH_OPEN_THRESHOLD && !isMouthOpen) {
            isMouthOpen = true;
            openingTimestamps.push(millis()); // Log the time of the new opening

            // Keep the timestamps array from growing too large. We only need the last 5.
            if (openingTimestamps.length > 5) {
              openingTimestamps.shift(); // Remove the oldest timestamp
            }
            
            // --- Check for FAST condition: 5 openings within 5 seconds ---
            if (openingTimestamps.length >= 5) {
              // Calculate the time difference between the newest and oldest timestamp in our list
              const timeSpan = openingTimestamps[openingTimestamps.length - 1] - openingTimestamps[0];
              if (timeSpan < 5000) {
                console.log("FAST condition met!");
                MtrSpd = 255; // Set to max speed
                openingTimestamps = []; // Reset after the gesture is recognized to avoid re-triggering
              }
            } else {
              // --- SLOW condition: Any single opening that doesn't meet the fast criteria ---
              console.log("SLOW condition met.");
              MtrSpd = 80; // Set to a slow speed
            }
            sendSpeedIfChanged(); // Send the new speed (fast or slow)

          } else if (mouthOpeningDistance < MOUTH_OPEN_THRESHOLD && isMouthOpen) {
            isMouthOpen = false; // Reset state when mouth closes.
          }

          // --- Visual Feedback ---
          // Flip back to draw text and shapes correctly
          translate(width, 0);
          scale(-1, 1);
          
          // Style similar to the user's original example
          noFill();
          stroke("yellow");
          strokeWeight(2);
          
          // Draw ellipses on the lips, scaled by the inter-ocular distance
          const ellipseSize = iod / 4; // Make the ellipse size proportional to the face size
          ellipse(upperLip.x, upperLip.y, ellipseSize, ellipseSize);
          ellipse(lowerLip.x, lowerLip.y, ellipseSize, ellipseSize);

          fill(255);
          stroke(0);
          strokeWeight(2);
          textSize(24);
          textAlign(LEFT, TOP);
          text(`Motor Speed: ${MtrSpd}`, 10, 10);
        }
      }

      // This function handles sending data to the serial port ONLY if it has changed.
      function sendSpeedIfChanged() {
        const speedToSend = int(MtrSpd);
        if (serial.isOpen() && speedToSend !== lastSentSpeed) {
          console.log(`Sending speed: ${speedToSend}`);
          serial.writeLine(speedToSend);
          lastSentSpeed = speedToSend;
        }
      }

      // ===== Helper functions below (exactly as in your example) =====

      function onSerialErrorOccurred(eventSender, error) {
        console.log("onSerialErrorOccurred", error);
        pHtmlMsg.html(error);
      }

      function onSerialConnectionOpened(eventSender) {
        console.log("onSerialConnectionOpened");
        pHtmlMsg.html("Serial connection opened successfully");
      }

      function onSerialConnectionClosed(eventSender) {
        console.log("onSerialConnectionClosed");
        pHtmlMsg.html("Serial connection closed");
      }

      function onSerialDataReceived(eventSender, newData) {
        console.log("onSerialDataReceived", newData);
        pHtmlMsg.html("onSerialDataReceived: " + newData);
      }

      function mouseClicked() {
        if (!serial.isOpen()) {
          serial.connectAndOpen(null, serialOptions);
        }
      }