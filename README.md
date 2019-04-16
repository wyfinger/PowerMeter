### Arduino PowerMeter 

This is a simple power meter for radio control engine test for arduino.

Board contains Arduino Nano, Nokia5130 display and ACS756 SCA-100B current sensor (Hall effect). This device can measure current (Ampere), voltage (Volt), power (Watt), time, max value of current  and power , min value of voltage, energy (Wh and Ah). In addition, the device also has a servo tester.

### Scheme
- Connect current sensor (I use ACS756 SCA-100B Holl based sensor) to A0 pin;
- Connect A1 pin to voltage divider center;
- Connect A2 pin to potentiometer center (for servo tester, if Your need);
- Connect display to digital pins, see [PowerMeter.ino#L14](https://github.com/wyfinger/PowerMeter/blob/master/PowerMeter/PowerMeter.ino#L14);
- Connect servo out to pin 10 (or any other).

#### Intefrace

![Screen1](https://github.com/wyfinger/PowerMeter/blob/imgs/Image1.jpg?raw=true)
![Screen1](https://github.com/wyfinger/PowerMeter/blob/imgs/Image2.jpg?raw=true)
![Screen1](https://github.com/wyfinger/PowerMeter/blob/imgs/Image3.jpg?raw=true)
![Screen1](https://github.com/wyfinger/PowerMeter/blob/imgs/Image4.jpg?raw=true)
