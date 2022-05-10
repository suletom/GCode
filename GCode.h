#ifndef GCode_h
#define GCode_h

#define GCODE_COMMAND_END '\n'
#define GCODE_MAXCMD 200

//pont struktura, kordinatakhoz
typedef struct p{
  long x;
  long y;
  long z;
} point;

//struktura egyenes vonalu mozgas koordinatainak szamitasahoz (Bresenham alg.)
typedef struct br{
  char dominant;
  char x_steep,y_steep,z_steep;
  long Dx,Dy,Dz;
  long ADx,ADy,ADz;
  long Dx2,Dy2,Dz2;
  long Fx,Fy,Fz;
  long err1,err2;
  long curr_step_pos;
} linehelper;

//struktura kor-hoz
typedef struct cd{
    long endx,endy; //vegpont
    long curx,cury; //aktualis helyzet
    long radius; //sugar
    long Fx,Fy;
    char sign1;
} circlehelper;

class GCode {

  private:
   unsigned int step_per_mm;
   unsigned int step_per_inch;
   unsigned int step_per_unit;
   unsigned int max_speed;
   unsigned int speed;
   unsigned char abs_mode;
   unsigned char plane;
   double feedrate;
   char currcommand;
   linehelper linedata;
   circlehelper circledata;
   char x_step;
   char y_step;
   char z_step;
   char half;
   point current_point;
   char lastg;
   char modspeedflag;
   
   unsigned long get_speed_by_feedrate();
   void calculate_line_steps();
   long _getcoord(long yx,long *lp);
   char _endcircle();
   void calculate_circle();
   unsigned char parse_typenum_to_byte(char * ch,unsigned char * pos);
   unsigned char parse_g_words(unsigned char num,char * ch);
   unsigned char parse_m_words(unsigned char num,char * ch);
   unsigned char check_word(char word,char * str);
   char toupper(char c);
   char isalpha(char c);
   void command_done();
   void setspeed(unsigned long speed);
   void modspeed();
   long get_words_nums(char word,char * str,char * err);	
   point _clm(long r, float x, float y);

  public:
    //konstruktor
    GCode(unsigned int step_per_mm,unsigned int max_speed);
    void init_line(long x0, long y0, long z0, long x1, long y1, long z1);
    void init_circle(long kx,long ky,long i,long j,long x,long y,char dir,long r);
    void calculate_steps();
    unsigned char parse_command(char cmd[]);
    char getXstep();
    char getYstep();
    char getZstep();
    char getmodspeed();
    unsigned long getspeed();
    char hasCommand();
    point getCurrentPoint();
    char isHalf();

};

#endif
