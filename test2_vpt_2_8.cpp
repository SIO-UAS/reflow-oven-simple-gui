#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Progress.H>
#include <FL/Fl_Simple_Counter.H>

#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <i2c/smbus.h>
#include <time.h>
#include <stdio.h>
#include <math.h>


using namespace std;
bool running = false;
float prog = 0.0;
char g_buffer[50];
char s_buf2[50];
bool coil1 = false;
int i2cfile;
bool i2cerr = false;
int i2cwordres = 0;
bool i2crerr = false;
float rcalibval = 75; // Calibration Resitance
double ptvala = 0.0039083; // °C^-1
double ptvalb = -0.0000005775; //°C^-2 to be found at the PT1000 Datasheet
//R(T) = R(0)*(1+A*T+B*T*T)
//-> T = (-B - sqrt(ptvalb*ptvalb - 4 * ptvala * R(0)-R(t))/(2*A)
double nomres = 1000; // Nominal Restiance
double tmax = 150;
int turnofft = 10; // Turnofftemperature = Tmax - turnofft
float waittime = 60; // Time it lasts
bool finished = false;


void process_a(void *ptr){
        Fl_Progress* bar = (Fl_Progress*)ptr;
	if(running == true){
	   if(prog>=100){
	    running = false;
	   }
	   else{
	   // prog++;
	   }
	 }
	else{
	  // prog = 0.0;
	}
	bar->value(prog);
	sprintf(g_buffer, "%3.0f%%", prog);
	bar->label(g_buffer);
	Fl::repeat_timeout(1.0, process_a, ptr);
}
//--------------------------------------------
void but_cb( Fl_Widget* o, void*  ) {
   Fl_Button* b=(Fl_Button*)o;
   if(running == false){
     b->label("Stop"); //redraw not necessary
     running = true;
    //Start process

   }
   else{
     b->label("Start");
     running = false;
     //(Stop process)
   }
   //b->resize(10,150,140,30); //redraw needed
   b->redraw();
}

//--------------------------------------------

void coilone_butcb(Fl_Widget* o, void*){
	Fl_Button* b=(Fl_Button*)o;
	if(coil1){
		b->label("Relay 1 EIN");
		coil1 = false;
		//I2C auschalt Code
		//i2c_smbus_write_quick( i2cfile, 0x02);
		char buf[1];
		buf[0] = 0x02;
		write(i2cfile, buf, 1);
	}
	else{
		b->label("Relay 1 Aus");
		coil1 = true;
		//I2C einschalt Code
		//i2c_smbus_write_quick(i2cfile, 0x01);
		char buf[1];
                buf[0] = 0x01;
                write(i2cfile, buf, 1);

	}

}

void i2creadword(){
	char buf[2];
	if (read(i2cfile, buf, 2) != 2) {
	  /* ERROR HANDLING: i2c transaction failed */
	 i2crerr = true;
	} else {


 	 i2cwordres = ((int)buf[0]<<2) + ((int)buf[1]>>6);
	i2crerr =false;
	}
	//printf("%X", i2cwordres);
	//Something is wrong with the reading idk if it is the PIC or the Pi have to figure out later f*** the two LSBs in the meantime should leave an inacuracy of 1.2°C
}


void timed_callback(void *ptr ){
  Fl_Box* bo = (Fl_Box*)ptr;
  time_t rawtime;
  struct tm * timeinfo;

  time (&rawtime);
  timeinfo = localtime (&rawtime);

  bo->label(asctime(timeinfo));
  printf("Time : %d", unsigned(time(0)));
  Fl::repeat_timeout(1.0, timed_callback, ptr);
}

double calctemp(){
	i2creadword();
        float milivolt = i2cwordres*3.22265; //ADC Result * (3300mV/1024bit) = mV mesaured
	// Volt to temp
        float preampv = (milivolt/1000)/(2*(100/4.7)); // Voltage before the Amplification 2 * (100k / 4k7) in Volt
        float sensorv = preampv + (3.3/51); // Differentail Voltage plus the Voltage of the other side of the voltage divider (because 50k+1k)
        float ptresi = (sensorv*50000)/(3.3*(1-sensorv)); //Resitance of PT1000, see calculations
	double resi = ptresi - rcalibval;
	double b = ptvalb;
	double a = ptvala;
	double temp = (nomres * a - sqrt((nomres*a*nomres*a)-4*(nomres*b)*(nomres-resi)))/(2*nomres*b*(-1));
	return temp;
}

void showtemp(void *ptr){
	Fl_Box* bo = (Fl_Box*)ptr;
	i2creadword();
	float milivolt = i2cwordres*3.22265; //ADC Result * (3300mV/1024bit) = mV mesaured
	// Volt to temp
	float preampv = (milivolt/1000)/(2*(100/4.7)); // Voltage before the Amplification 2 * (100k / 4k7) in Volt
	float sensorv = preampv + (3.3/51); // Differentail Voltage plus the Voltage of the other side of the voltage divider (because 50k+1k)
	float ptresi = (sensorv*50000)/(3.3*(1-sensorv)); //Resitance of PT1000, see calculations
	sprintf(s_buf2, "%f °C", calctemp());
	bo -> label(s_buf2);
	Fl::repeat_timeout(1.0, showtemp, ptr);


}

void regel(void *ptr){
	Fl_Simple_Counter* tset = (Fl_Simple_Counter*) ptr;
	tmax = tset->value();
	double  curtemp = calctemp();
	if(running){
		if(curtemp < tmax- turnofft){
			// Turnon
			if(coil1){
			prog = (curtemp /(tmax-turnofft))*100.0 -20.0;
			}
			else{
				coil1 = true;
				char buf[1];
                                buf[0] = 0x01;
                                write(i2cfile, buf, 1);
			}
		}
		else{
			//Turnoff
			if (finished == false){
				finished = true;
				char buf[1];
                		buf[0] = 0x02;
                		write(i2cfile, buf, 1);
				prog = 90;
				Fl::repeat_timeout(waittime, regel, ptr);
				return;
			}
			else{
				prog = 100.0;
				running =false;
			}
		}
	}
	else{
	}
	Fl::repeat_timeout(1.0, regel, ptr);
	return;



}


int main() {
    //I2C Setup
    i2cfile = open("/dev/i2c-11", O_RDWR);
    if(i2cfile < 0){
	i2cerr = true;
    }
    if(ioctl(i2cfile, I2C_SLAVE, 0x08) < 1){
	i2cerr = true;
    }

    //Fltk Code
    Fl_Window win( 480,700 ,"Ofen Test" );
    win.begin();
    Fl_Button but( 140, 600, 200, 100, "Start" );
    Fl_Box box(50, 50, 100, 30, "*time*");
    Fl_Progress bar(20,500, 440, 30, "0%");
    Fl_Simple_Counter tset( 20, 400, 440, 50, "Max. Temp");
    Fl_Button coil1( 20, 320, 200, 50, "Relay 1 EIN");
    Fl_Box tempshow(100, 250, 300, 50, "I2C Read Data");
    win.end();
    but.labelsize(30);
    bar.labelsize(20);
    bar.maximum(100.0);
    bar.minimum(0.0);
    bar.value(0.0);
    bar.selection_color(FL_CYAN);
    but.callback( but_cb );
    coil1.callback( coilone_butcb );
    tset.step(5.0,5.0);
    tset.value(150.0);
    tset.precision(1);
    tset.range(50.0, 280.0);
    win.show();
    Fl::add_timeout(1.0, timed_callback, &box);
    Fl::add_timeout(1.0, process_a, &bar);
    Fl::add_timeout(1.0, showtemp, &tempshow);
    Fl::add_timeout(1.0, regel, &tset);
    return Fl::run();
}
