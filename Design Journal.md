# Liv's Design Journal

Hello and welcome to my design journal. Hopefully this will be a clear representation of everything I have been doing throughout TDF during Fall 2025 and will be an enjoyable read!

I have divided this into a few sections:
1. [The Arduino](#arduino)
2. [Laser Cutter](#laser-cutter)

## The Arduino {#arduino}

### Week 1
Never have I ever touched an arduino. Nope. This is so true. So I must admit I was a bit intimidated when we first got our kits filled with electronics. But all the guides were super useful (except for the fact that I hadn't realised I had to load my code to the arduino so I kept pressing run and nothing happened). Finally, I realised I had to do that and magical things started happening!

First off, my arduino came with Blink loaded so as soon as I connected it, it was doing its thing (I thought it might be an error message but no, it was just blink - thank you google for answering my questions).

So step one was forcing off the light:
```arduino
void setup() {
 
  // do nothing except force 13 led off
  pinMode(13, OUTPUT);
  digitalWrite(13, LOW);
}
void loop() {
}
```

Crazy stuff, I know. 

So then we got to make the light blink:
```arduino
int led = 13;  // define a variable to hold the pin number of the internal LED

// the setup function runs once when you press reset or power the board
void setup() {
  // initialize digital pin LED_BUILTIN as an output.
  pinMode(led, OUTPUT);
}

// the loop function runs over and over again forever
void loop() {
  digitalWrite(led, HIGH);   // turn the LED on (HIGH is the voltage level)
  delay(1000);                       // wait for a second
  digitalWrite(led, LOW);    // turn the LED off by making the voltage LOW
  delay(1000);                       // wait for a second
}
```

And I connected an LED to it!

![Alt text](/images/week%201/blinkwithled.png)

Proceeded to try changing it to pin 8 and then pin 8 and 13 alternatingly! (not all code is included here as they are just variations of each other but it can be found on the github folder this markdown belongs to)

```arduino
int led = 13;  // define a variable to hold the pin number of the internal LED
int ledext = 8;
// the setup function runs once when you press reset or power the board
void setup() {
  // initialize digital pin LED_BUILTIN as an output.
  pinMode(led, OUTPUT);
  pinMode(ledext, OUTPUT);
}

// the loop function runs over and over again forever
void loop() {
  digitalWrite(led, HIGH);   // turn the LED on (HIGH is the voltage level)
  digitalWrite(ledext, LOW);
  delay(1000);                       // wait for a second
  digitalWrite(led, LOW);    // turn the LED off by making the voltage LOW
  digitalWrite(ledext, HIGH);
  delay(1000);                       // wait for a second
}
```

You can see the video here: https://photos.app.goo.gl/9VgwX1SNipQTziiv8
________________________________________________________________________
Homework for this week:
- Write  a program to flash the onboard LED while printing Hello World to serial

```arduino
int led = 13;  // define a variable to hold the pin number of the internal LED

void setup() {
  // put your setup code here, to run once:

  // initialize digital pin LED_BUILTIN as an output.
  pinMode(led, OUTPUT);
  Serial.begin(9600);
}

void loop() {
  // put your main code here, to run repeatedly:
//send 'Hello, world!' over the serial port
  Serial.println("Hello, world!");
  delay(1000);
  digitalWrite(led, HIGH);   // turn the LED on (HIGH is the voltage level)
  delay(1000);                       // wait for a second
  digitalWrite(led, LOW);    // turn the LED off by making the voltage LOW
  delay(1000);                       // wait for a second
}
```

- Make “something interesting” happen with:

1 LEDs + 1 LDR
2 or more LEDs

Ok Sudhu, an open-ended exercise... it's always tough to just try something out with a new medium BUT let me think of something.

So I knew what an LED was but not an LDR (the abbreviation at least). My first step was to test it out. I didn't change my resistor like it was indicated on Sudhu's github page on arduinos but it still worked and made me understand what this magical new tool does (refer to video link below). I used the flashlight on my phone to shed even more light on the resistor and got it up to 41 I believe. 

You can see the video here:
https://photos.app.goo.gl/dGQg98hR28bRwcBfA

So now I am home, after doing all these things during Sudhu's lesson and being exceedingly proud of myself (image evidence below).

Yep, it went on the family groupchat :)

![]![alt text](/images/week%201/image.png)

So I tested out the examples you had up, this is one of them: https://photos.app.goo.gl/qhkP5B8pzVw5FaAp8

I think I want to do something that reads an input from the LDR and tells you how bright a 'room' is depending on the value. I drew this out, having the LEDs in parallel. I used gemini to help me create this code but it needed troubleshooting anyway:

```arduino

// Define the pins for your colored LEDs
const int ldrPin = A0;
const int redLedPin = 2;    // For "pitch black"
const int yellowLedPin = 3; // For "ambient"
const int greenLedPin = 4;  // For "super bright"


// Use the Serial Monitor to find the best numbers.
// More light = lower LDR value.
const int brightThreshold = 250; // above this value, it's bright
const int darkThreshold = 150;   // below this value, it's pitch black

void setup() {
  // Set all LED pins as outputs
  pinMode(redLedPin, OUTPUT);
  pinMode(yellowLedPin, OUTPUT);
  pinMode(greenLedPin, OUTPUT);
  
  // Start serial communication so you can find your threshold values
  Serial.begin(9600); 
}

void loop() {
  // Read the value from the LDR sensor
  int ldrValue = analogRead(ldrPin);
  
  // Print the current light value to help you calibrate
  Serial.print("LDR Value: ");
  Serial.println(ldrValue); 
  
  // Logic to turn on the correct LED
  if (ldrValue > brightThreshold) { // Super bright conditions
    digitalWrite(greenLedPin, HIGH);
    digitalWrite(yellowLedPin, LOW);
    digitalWrite(redLedPin, LOW);
  } 
  else if (ldrValue < darkThreshold) { // Pitch black conditions
    digitalWrite(greenLedPin, LOW);
    digitalWrite(yellowLedPin, LOW);
    digitalWrite(redLedPin, HIGH);
  } 
  else { // Ambient light (in between bright and dark)
    digitalWrite(greenLedPin, LOW);
    digitalWrite(yellowLedPin, HIGH);
    digitalWrite(redLedPin, LOW);
  }
  
  delay(100); // A small delay for stability
}

```
So I had tested in the previous exercise what the brightness output was when in my ambient (200ish), if I put my finger over it (100ish) and if I shined a light on it (600 to 800) so I knew I had to make those my threshold values.

Video here: https://photos.app.goo.gl/CKZZgYVm32XcnNbw9

Annnndddddd so now I wanted to try and combine the two and make it dim or not depending on how bright it is and also change colours for different thresholds.

This is a pic of the circuit for the lights:
![alt text](/images/week%201/circuit3Leds.png)

LLMs are useful because when things don't work, you can usually get help: "Move the LEDs to PWM-capable pins. On most Arduino boards, these are marked with a tilde (~).".

So great, we figured that out. Let's try again.

```arduino
const int ldrPin = A0;
const int redLedPin = 9; 
const int yellowLedPin = 10;
const int greenLedPin = 11;  

// --- CALIBRATION THRESHOLDS ---
// More light = higher LDR value.
const int brightThreshold = 150;
const int darkThreshold = 550;

void setup() {
  // Set all LED pins as outputs
  pinMode(redLedPin, OUTPUT);
  pinMode(yellowLedPin, OUTPUT);
  pinMode(greenLedPin, OUTPUT);

  Serial.begin(9600); 
}

void loop() {
  // Read the value from the LDR sensor
  int ldrValue = analogRead(ldrPin);
  
  // Print value
  Serial.println(ldrValue); 
  
  // Logic to turn on the correct LED with variable brightness
  if (ldrValue < brightThreshold) { // Super bright conditions
    // The GREEN LED gets brighter as the light increases (ldrValue goes up)
    int brightness = map(ldrValue, brightThreshold, 0, 0, 255);
    analogWrite(redLedPin, brightness);
    analogWrite(yellowLedPin, 0); // Turn other LEDs off
    analogWrite(greenLedPin, 0);
  } 
  else if (ldrValue > darkThreshold) { // Pitch black conditions
    // The RED LED gets brighter as it gets darker (ldrValue decreases)
    int brightness = map(ldrValue, darkThreshold, 1023, 0, 255);
    analogWrite(greenLedPin, brightness);
    analogWrite(redLedPin, 0); // Turn other LEDs off
    analogWrite(yellowLedPin, 0);
  } 
  else { // Ambient light (in between bright and dark)
    // The YELLOW LED fades in and out within the ambient range
    int brightness = map(ldrValue, brightThreshold, darkThreshold, 0, 255);
    analogWrite(yellowLedPin, brightness);
    analogWrite(greenLedPin, 0); // Turn other LEDs off
    analogWrite(redLedPin, 0);
  }
  
  delay(10); // A smaller delay for smoother transitions
}

```


I had to edit the code as I had a pull down, not a pull up resistor. 

BUT WE GOT THERE: TADAAAAAA https://photos.app.goo.gl/VDqCrcfi39f5Pdtu9

We did it! I think I understand this whole jazz a bit better now. And in the end, you really need to understand what you are doing in order to set it up correctly aand be able to troubleshoot. I liked that we got to play around with it and just figure it out.

## Laser Cutters {#laser-cutter}

### Week 1

Week 1 with Chris... the topic? lasers. Automatically, cool. 

I've had so many laser cutting trainings but I had never actually gotten my hands dirty without supervision - it felt like a lot of responsibility. It was great to see the process shown in a clear yet simple way. I have now requested the "Manufacturing Processes for design professionals" on loan from the library (it was too expensive to buy). 

Thank you for the demo during class, it came in very handy.

I first had to think whether I wanted to do a cause I believed in or a personality thing. Recently, I have been thinking about my identity - it is a slightly turbulent topic for me because it is so all over the place (gegraphically and culturally). One of the roots of this is because I have lived in 4 different countries and I identify with a lot of parts of each culture. For this assignment, I thought I could tackles this and make a statement that I carry part of all of them with me!

So I started trying to come up with some ideas:

![alt text](/images/week%201/sketches.png)

I setlled on wanting to represent my different roots but also do something that could teach me how to raster, engrave, and cut. This was perfect. 

I love animals and am passionate about having sanctuaries instead of zoos, and adoption in general for any kind of pet. 

So animals and countries (sounds like a 1st grade book title).

Courtesy of Gemini (notice the eiffel tower in Russia, Asia in Grreenland, two Australias and the US as Africa among other things)

![alt text](/images/week%201/joke.png)

So I found images of animals online and trying to image trace them. Some image traces were bad so I started playing with the different setting. Outline worked well, black and white logo for some things and certain images worked better than others. 

![alt text](/images/week%201/toucan.png)

I measured the circumference of my fingers roughly with string and a ruler and divided by pi. 

Oh, and then converted to inches because I'm holding onto my metric girly title like it's my life.

After working out some kinks with adobe illustrator (arguably, the worse AI), I started learning some shortcuts and became a wizard (in comparison to how slow I was at the start). I also determined the ring's thickness would be 2mm because I forgot that laser + close together lines is bad.

So this was the first sketch:
![alt text](/images/week%201/firstsketch.png)

I went to print, did everything correctly:
![alt text](/images/week%201/emptyboard.png)

But this came out:

![alt text](/images/week%201/firstprint.png)


So clearly my outer and inner diameter difference of 2mm was too little and I also could have made sure my print properly fit in the area I wanted to print so I didn't just throw the laser on the bed. I started pointing my laser after this to the most extreme edges to make sure the plywood was not already cut in that area.

I widened the diameters, and tried printing again. 

![alt text](/images/week%201/secondprint.png)

This time, the width was better but some of my circles were not centered. I tried wearing them and the horse and bear rings were too wide. Additionally, the connections between these animals and the ring were too thin and they easily broke off.

Therefore, I widened the connections, ensured the radius difference was consistent and that the circles were centered. At this point I also did a Chris and forgot to select the material so had to do two passes.

Ta-da:
![alt text](/images/week%201/thirdcut.png)

Some circles were still not super centered and the horse connection was too weak. 

So again, I tried BUT THIS TIME IT WORKED:

![alt text](/images/week%201/finalcut.png)

I sanded it and these were the rings I ended up making:

![alt text](/images/week%201/thend.png)

________________________________________________________________________

