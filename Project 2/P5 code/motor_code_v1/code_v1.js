
      let pHtmlMsg;
      let serialOptions = { baudRate: 115200 };
      let serial;
      let MtrSpd = 0;
      let lastSentSpeed = -1;

      let video;
      let faceMesh;
      let faces = [];

      const MOUTH_OPEN_THRESHOLD = 15; // Sensitivity for mouth opening. Adjust as needed.
      const MEASUREMENT_INTERVAL = 1000; // The time window in milliseconds (1 second) to measure frequency.
      let mouthOpenings = 0; // Counter for events within the time window.
      let isMouthOpen = false; // State variable to detect a single open event.

      function preload() {
        // Load the faceMesh model instead of bodyPose
        faceMesh = ml5.faceMesh();
      }

      function setup() {
        createCanvas(640, 480);

        video = createCapture(VIDEO);
        video.size(width, height);
        video.hide();

        faceMesh.detect(video, gotFaces);

        serial = new Serial();
        serial.on(SerialEvents.CONNECTION_OPENED, onSerialConnectionOpened);
        serial.on(SerialEvents.CONNECTION_CLOSED, onSerialConnectionClosed);
        serial.on(SerialEvents.DATA_RECEIVED, onSerialDataReceived);
        serial.on(SerialEvents.ERROR_OCCURRED, onSerialErrorOccurred);

        // Attempt to auto-connect to a previously approved port
        serial.autoConnectAndOpenPreviouslyApprovedPort(serialOptions);

        // Add a <p> element for messages
        pHtmlMsg = createP(
          "Click anywhere on this page to open the serial connection dialog"
        );
        pHtmlMsg.style("color", "deeppink");

        // Set interval to periodically calculate frequency and send speed
        setInterval(calculateAndSendSpeed, MEASUREMENT_INTERVAL);
      }

      // Callback for when ml5 finds faces
      function gotFaces(newFaces) {
        faces = newFaces;
        // Re-run the detection immediately- creates a continuous loop
        faceMesh.detect(video, gotFaces);
      }

      function draw() {
        // Mirror the video feed for a more natural feel
        translate(width, 0);
        scale(-1, 1);
        image(video, 0, 0, width, height);

        if (faces.length > 0) {
          const keypoints = faces[0].keypoints;

          // Landmarks for the upper and lower inner lip
          const upperLip = keypoints[13];
          const lowerLip = keypoints[14];
          const mouthOpeningDistance = dist(
            upperLip.x,
            upperLip.y,
            lowerLip.x,
            lowerLip.y
          );

          if (mouthOpeningDistance > MOUTH_OPEN_THRESHOLD && !isMouthOpen) {
            isMouthOpen = true;
            mouthOpenings++; // Increment the counter for this interval.
          } else if (
            mouthOpeningDistance < MOUTH_OPEN_THRESHOLD &&
            isMouthOpen
          ) {
            isMouthOpen = false; // Reset state when mouth closes.
          }

          translate(width, 0);
          scale(-1, 1);
          fill(0, 255, 0);
          noStroke();
          ellipse(upperLip.x, upperLip.y, 10, 10);
          ellipse(lowerLip.x, lowerLip.y, 10, 10);

          fill(255);
          stroke(0);
          strokeWeight(2);
          textSize(24);
          textAlign(LEFT, TOP);
          text(`Motor Speed: ${MtrSpd}`, 10, 10);
        }
      }

      //runs every MEASUREMENT_INTERVAL
      function calculateAndSendSpeed() {
        // --- Frequency Calculation and Mapping ---
        // The number of `mouthOpenings` is the frequency for the last second.
        // We map this frequency (e.g., 0 to 5 openings per second) to a motor speed range (0-255).
        MtrSpd = map(mouthOpenings, 0, 5, 0, 255);
        MtrSpd = constrain(MtrSpd, 0, 255);
        MtrSpd = int(MtrSpd);

        // Only send data if speed has changed and port is open
        if (serial.isOpen() && MtrSpd !== lastSentSpeed) {
          console.log(`Sending speed for ${mouthOpenings} openings/sec:`, MtrSpd);
          serial.writeLine(MtrSpd);
          lastSentSpeed = MtrSpd;
        }

        // Reset the counter to begin measuring for the next time interval.
        mouthOpenings = 0;
      }

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