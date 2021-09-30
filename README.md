# reflow-oven-simple-gui
 A GUI App for my Reflowoven.
 
 Not very clean code, not a very good style. (My first time working with FLTK, I2C and void pointers). Yet it seems to work.( Haven't test this version yet)
 Runs on an RPI 3
 # How to use:
  Install FLTK Libs:
   ```bash
   sudo apt install fltk1.3-dev
  ```
 Clone the repository
 
 Compile with: 
   ```bash
   g++ test2_vpt_2_8.cpp -lfltk -li2c -o 'Name of the executable'
   ```
 # To Do's
- [x] Control Relay with the IO-Board
- [x] Read the Sensor
- [ ] Read the Sensor right (full 10bit)
- [ ] Format Code
- [ ] Test Code if the 'control loop' works 
- [ ] Work on the GUI
- [ ] Make a Object Orientet version

