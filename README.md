The openocd reset command does not work with this code. I don't know why but a
workaround is to just load the code with: 
`openocd -f interface/raspberrypi-swd.cfg -f target/rp2040.cfg -c "program alarm-clock.elf verify exit"`
and then unplugg/replugg the power to the pico.
