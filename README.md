# Power-Supply

This is a homemade linear bench power supply using an LM358 and a TIP120 darlington pair transistor as a series pass transistor.
The power supply is capable of 0-25V and 0-2.5A and has an over current/constant current protection feature. It also has an over thermal protection shutoff.
A MAX4376TASA+ is used with a current sense resistor for high side current sensing.
The Arduino uses its ADC to measure voltages and currents with a 16x2 LCD display for the output.

The documentation is by no means close to done and has little to no organization and I have yet to test the supply in a lab to get exact specifications for
output ripple voltages.
