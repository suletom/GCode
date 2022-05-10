#include "Arduino.h"
#include "GCode.h"

GCode::GCode(unsigned int step_per_mm,unsigned int max_speed){
	GCode::step_per_mm=step_per_mm;
	GCode::step_per_inch=(double)step_per_mm*25.4;
	GCode::max_speed=max_speed;
	
        GCode::x_step=0;
   	GCode::y_step=0;
   	GCode::z_step=0;

	GCode::current_point.x=0;
	GCode::current_point.y=0;
  	GCode::current_point.z=0;

	GCode::modspeedflag=0;
	GCode::speed=max_speed;
	GCode::half=0;
	GCode::currcommand=0;
}

//vonal bejarasanak megkezdese adott pontbol, az uj pontba
void GCode::init_line(long x0, long y0, long z0, long x1, long y1, long z1){
	
    GCode::half=0;

    //ha minden iranyba 0, akkor nincs ertelme az egesznek
    
    //innetol tudja a szamolo fuggveny h vonalat kell szamolni
    GCode::currcommand=1;
    
    //megtett lepesek szama, h tudjuk h mikor vegeztunk
    GCode::linedata.curr_step_pos=0;
    
    //a felírandó vonalhoz tartozó háromszög merőleges oldalai hosszának számításaGCode::half=0;
    GCode::linedata.Dx = x1 - x0;
    GCode::linedata.Dy = y1 - y0;
    GCode::linedata.Dz = z1 - z0;
 
    //ha mind 0, akor azt mondjuk hogy keszvagyunk (pl. tul kicsi koordinatak eseten a kerekites miatt lehet 0,0,0)
    if (GCode::linedata.Dx==0 && GCode::linedata.Dy==0 && GCode::linedata.Dz==0){
      GCode::command_done();
      return;
    }

    //'letaroljuk az elojeleket'
    if (GCode::linedata.Dx>0){
        GCode::linedata.x_steep=1;
    }else{
        GCode::linedata.x_steep=-1;
    }
    if (GCode::linedata.Dy>0){
        GCode::linedata.y_steep=1;
    }else{
        GCode::linedata.y_steep=-1;
    }
    if (GCode::linedata.Dz>0){
        GCode::linedata.z_steep=1;
    }else{
        GCode::linedata.z_steep=-1;
    }

    GCode::linedata.ADx=abs(GCode::linedata.Dx);
    GCode::linedata.ADy=abs(GCode::linedata.Dy);
    GCode::linedata.ADz=abs(GCode::linedata.Dz);

    GCode::linedata.Dx2=GCode::linedata.ADx*2;
    GCode::linedata.Dy2=GCode::linedata.ADy*2;
    GCode::linedata.Dz2=GCode::linedata.ADz*2;

    //ha x tengelyen mozdultunk el a legtobbet
    if ((GCode::linedata.ADx>=GCode::linedata.ADy) && (GCode::linedata.ADx>=GCode::linedata.ADz)){
        GCode::linedata.dominant=0;
        GCode::linedata.err1=GCode::linedata.Dy2-GCode::linedata.ADx;
        GCode::linedata.err2=GCode::linedata.Dz2-GCode::linedata.ADx;
    }
    //ha y tengelyen mozdultunk el a legtobbet
    if ((GCode::linedata.ADy>=GCode::linedata.ADx) && (GCode::linedata.ADy>=GCode::linedata.ADz)){
        GCode::linedata.dominant=1;
        GCode::linedata.err1=GCode::linedata.Dx2-GCode::linedata.ADy;
        GCode::linedata.err2=GCode::linedata.Dz2-GCode::linedata.ADy;
    }
    //ha z tengelyen mozdultunk el a legtobbet
    if ((GCode::linedata.ADz>=GCode::linedata.ADx) && (GCode::linedata.ADz>=GCode::linedata.ADy)){
        GCode::linedata.dominant=2;
        GCode::linedata.err1=GCode::linedata.Dx2-GCode::linedata.ADz;
        GCode::linedata.err2=GCode::linedata.Dy2-GCode::linedata.ADz;
    }

    // LASSITAShoz felezopont szamitasa
    GCode::linedata.Fz=GCode::linedata.ADz/2;
    GCode::linedata.Fx=GCode::linedata.ADx/2;
    GCode::linedata.Fy=GCode::linedata.ADy/2;
    //Serial.print(    GCode::linedata.Fx);
}
// i,j: iv kozeppontjanak x,y koorditaja
// x,y: aktualis pozicio (kezdopont)
void GCode::init_circle(long kx, long ky,long i,long j,long x,long y,char dir,long r){
char s1,s2,b=0;
float m;
long tmp,tmp1;
float tmp2,tmp3;
   
    GCode::half=0;
    
    if (dir) GCode::circledata.sign1=1;
    else GCode::circledata.sign1=-1;  //G03 ellentetesen

    //eltolas -> iv kozeppontja lesz az origo
    x=x-i-kx;
    y=y-j-ky;

    //ezek a koriv éppen aktuális pontja (kezdopont), kozeppont az origo
    GCode::circledata.curx=-i;
    GCode::circledata.cury=-j;

    //sugar szamitasa
    GCode::circledata.radius=round(sqrt((i*i)+(j*j)));

    //vegpont koordinatainak szamitasa
    GCode::circledata.endx=x;
    GCode::circledata.endy=y;
    

    point tmpp = GCode::_clm(GCode::circledata.radius,GCode::circledata.endx,GCode::circledata.endy);
    GCode::circledata.endx=tmpp.x;
    GCode::circledata.endy=tmpp.y;
	
  
//felezopont
tmpp.x=((GCode::circledata.endx+GCode::circledata.curx)/2);
tmpp.y=((GCode::circledata.endy+GCode::circledata.cury)/2);

tmpp = GCode::_clm(GCode::circledata.radius,tmpp.x,tmpp.y);

//GCode::circledata.sign1
GCode::circledata.Fx=tmpp.x*GCode::circledata.sign1;
GCode::circledata.Fy=tmpp.y*GCode::circledata.sign1;

 //beallitjuk hogy a paranccsal kort csinalunk
 GCode::currcommand=2;
 
}
/*
 * Kör és sugár irányú egyenes metszespontja
 * r: sugar, y: egyenes egy pontja , x: egyenes egy pontja
 */
point GCode::_clm(long r, float x, float y) {
char b=0;
float m=0;
point pt;
char s1,s2;

    if (x < 0) s1=-1;
    else s1=1;

    if (y < 0) s2=-1;
    else s2=1;

    if (x==0 && y==0){
        b=1;
    }

    if (x==0){
	//0 -> 0val valo osztas
	x=0;
	y=r;
	b=1;
    }

    if (y==0){
	//vegtelen -> tulcsordulas
	y=0;
	x=r;
	b=1;
    }

    if (!b){

        //egyenes egyenletehez.... meredekseg szamitasa
        m=(float)(y/(float)x);

        // kiszamoljuk a vegpont altal meghatarozott egyenes es a kor metszespontjanak koordianatait
        // ez esetben y-hoz szamoljuk az x-et
        if (m<=1 && m>=-1){
            y=round(sqrt((float)((float)-4*(1+ ((float)1/(float)(m*m)) ) ) * -1*(r*r) ) / (float)((float)(2+2/(m*(float)m))));
            x=round(sqrt((r*r)-(y*y)));

        }
        else{
            x=round(sqrt((-4-(float)4*(float)(m*(float)m)) *(float)(-1)* (float)(r*r) ) / (float)((float)(2+2*(float)(m*(float)m))));
            y=round(sqrt((r*r)-(x*x)));

	}

    }
    
    pt.x=x*s1;
    pt.y=y*s2;
    pt.z=0;

    return pt;
}

void GCode::setspeed(unsigned long speed){
    
    GCode::speed=speed;
    
} 

void GCode::modspeed(){
    GCode::modspeedflag=1;
} 


unsigned long GCode::get_speed_by_feedrate(){

    unsigned long tmp;  
	//feedrate: units per minute (pl. mm/perc),mar eleve lebontva kapjuk a feedreate erteket, 
	// step_per_unit ertekevel szorozva
	//steps/minute: (feedrate*step_per_unit)       
	//1000000 usec -> 1 sec
	tmp=( (60/(GCode::feedrate))*1000000 );

	//ha gyorsabb mint a max, akkor a max lesz a sebesseg
	if (tmp<GCode::max_speed) return GCode::max_speed;
	else return tmp;
	
}

void GCode::command_done(){
  
   GCode::currcommand=0;
   
}

//eldontjuk hogy melyik irany(ok)ba kell lepnunk vonal eseten
void GCode::calculate_line_steps(){
  GCode::linedata.curr_step_pos++;
  
  switch(GCode::linedata.dominant){
     case 0:
       
	
       if (GCode::linedata.curr_step_pos==(GCode::linedata.Fx)){
	   GCode::half=1;
	   //Serial.print("m5");
	   //Serial.print(GCode::half);
       }
	
       
           GCode::x_step=GCode::linedata.x_steep;
       if (GCode::linedata.err1>0){
         GCode::y_step=GCode::linedata.y_steep;
         GCode::linedata.err1-=GCode::linedata.Dx2;
       }
       if (GCode::linedata.err2>0){ 
         GCode::z_step=GCode::linedata.z_steep;
         GCode::linedata.err2-=GCode::linedata.Dx2;
       }
       GCode::linedata.err1+=GCode::linedata.Dy2;
       GCode::linedata.err2+=GCode::linedata.Dz2;
	
	   if (GCode::linedata.curr_step_pos==(GCode::linedata.ADx)){
           GCode::x_step=GCode::linedata.x_steep;
           GCode::command_done();
           return;
       }

     break;
     case 1:

       if (GCode::linedata.curr_step_pos==(GCode::linedata.Fy)){
	   GCode::half=1;
       }

           GCode::y_step=GCode::linedata.y_steep;
       if (GCode::linedata.err1>0){
         GCode::x_step=GCode::linedata.x_steep;
         GCode::linedata.err1-=GCode::linedata.Dy2;
       }
       if (GCode::linedata.err2>0){ 
         GCode::z_step=GCode::linedata.z_steep;
         GCode::linedata.err2-=GCode::linedata.Dy2;
       }
       GCode::linedata.err1+=GCode::linedata.Dx2;
       GCode::linedata.err2+=GCode::linedata.Dz2;
	
       if (GCode::linedata.curr_step_pos==(GCode::linedata.ADy)){
           GCode::y_step=GCode::linedata.y_steep;
           GCode::command_done();
           return;
       }	

     break;
     case 2:

       if (GCode::linedata.curr_step_pos==(GCode::linedata.Fz)){
	   GCode::half=1;
       }

           GCode::z_step=GCode::linedata.z_steep;
       if (GCode::linedata.err1>0){
         GCode::y_step=GCode::linedata.y_steep;
         GCode::linedata.err1-=GCode::linedata.Dz2;
       }
       if (GCode::linedata.err2>0){ 
         GCode::x_step=GCode::linedata.x_steep;
         GCode::linedata.err2-=GCode::linedata.Dz2;
       }
       GCode::linedata.err1+=GCode::linedata.Dy2;
       GCode::linedata.err2+=GCode::linedata.Dx2;

	   if (GCode::linedata.curr_step_pos==(GCode::linedata.ADz)){
           GCode::z_step=GCode::linedata.z_steep;
           GCode::command_done();
           return;
       }    
 
     break;
    
  }
}

char GCode::isHalf(){
  //Serial.print(GCode::half); 
  char tmp=GCode::half;
  GCode::half=0;
  return tmp;
}

long GCode::_getcoord(long yx,long *lp){
long tmp=round(sqrt((GCode::circledata.radius*GCode::circledata.radius)-(yx*yx)) );

      if ((*lp)<0) {
          if ((*lp)!=-tmp){
             (*lp)=-tmp;
             return 1;
          }else{
             (*lp)=-tmp;
             return 0;
          }
      }
      else {
        if ((*lp)!=tmp){
             (*lp)=tmp;
             return 1;
          }else{
             (*lp)=tmp;
             return 0;
          }
      }
         
return 0;    
}

char GCode::_endcircle(){
 
  GCode::x_step=GCode::x_step*GCode::circledata.sign1;
  GCode::y_step=GCode::y_step*GCode::circledata.sign1;

  if (GCode::circledata.curx==GCode::circledata.endx && GCode::circledata.cury==GCode::circledata.endy){
    //vegeztunk, nem kell mar kort szamolni
    GCode::command_done();
  }
  
  if (GCode::circledata.curx==GCode::circledata.Fx && GCode::circledata.cury==GCode::circledata.Fy){
    GCode::half=1;
  }
}

void GCode::calculate_circle(){


//elso 4-ed 
if (GCode::circledata.curx > 0 && GCode::circledata.cury > 0){
    //elso 8-ad
    if (GCode::circledata.curx<GCode::circledata.cury){
	GCode::circledata.curx+=GCode::circledata.sign1;
        GCode::x_step=1;
	GCode::y_step=-GCode::_getcoord(GCode::circledata.curx,&GCode::circledata.cury);

    }else{ //masodik 8-ad
	GCode::circledata.cury+=-GCode::circledata.sign1;
        GCode::y_step=-1;
	GCode::x_step=GCode::_getcoord(GCode::circledata.cury,&GCode::circledata.curx);
    }
    GCode::_endcircle();
    return ;
}
//masodik 4-ed 
if (GCode::circledata.curx > 0 && GCode::circledata.cury <= 0){
    //elso 8-ad
    if (GCode::circledata.curx>abs(GCode::circledata.cury)){
	GCode::circledata.cury+=-GCode::circledata.sign1;
        GCode::y_step=-1;
	GCode::x_step=-GCode::_getcoord(GCode::circledata.cury,&GCode::circledata.curx);
    }else{ //masodik 8-ad
	GCode::circledata.curx+=-GCode::circledata.sign1;
        GCode::x_step=-1;
	GCode::y_step=-GCode::_getcoord(GCode::circledata.curx,&GCode::circledata.cury);
    }
    GCode::_endcircle();
    return ;
}
//harmadik 4-ed 
if (GCode::circledata.curx <= 0 && GCode::circledata.cury < 0){
    //elso 8-ad
    if (abs(GCode::circledata.curx)<abs(GCode::circledata.cury)){
	GCode::circledata.curx+=-GCode::circledata.sign1;
        GCode::x_step=-1;
	GCode::y_step=GCode::_getcoord(GCode::circledata.curx,&GCode::circledata.cury);
    }else{ //masodik 8-ad
	GCode::circledata.cury+=GCode::circledata.sign1;
        GCode::y_step=1;
	GCode::x_step=-GCode::_getcoord(GCode::circledata.cury,&GCode::circledata.curx);
    }
    GCode::_endcircle();
    return ;
}
//negyedik 4-ed 
if (GCode::circledata.curx <= 0 && GCode::circledata.cury >= 0){

	//nagyon pici kor kezelese, ilyenkor nem leptethetunk, mert az vegtelenciklus lenne :)
	if (GCode::circledata.curx==GCode::circledata.cury && GCode::circledata.curx==0){
		GCode::x_step=0;
		GCode::_endcircle();
    	return ;
	}

    //elso 8-ad
    if (abs(GCode::circledata.curx)>GCode::circledata.cury){
	GCode::circledata.cury+=GCode::circledata.sign1;
        GCode::y_step=1;
	GCode::x_step=GCode::_getcoord(GCode::circledata.cury,&GCode::circledata.curx);
    }else{ //masodik 8-ad
	GCode::circledata.curx+=GCode::circledata.sign1;
        GCode::x_step=1;
	GCode::y_step=GCode::_getcoord(GCode::circledata.curx,&GCode::circledata.cury);
    }
    GCode::_endcircle();
    return ;
}

}

//mondattípusokat azonosító számok ellenőrzése/konvertálása
byte GCode::parse_typenum_to_byte(char * ch,byte * pos){
  
  if ((*ch)>='0' && (*ch)<='9' && ( *(ch+1)<'0' || *(ch+1)>'9') ){
      *pos=2;
      return ((*ch)-'0');
  }
  
  if ((*ch)>='0' && (*ch)<='9' && (*(ch+1))>='0' && (*(ch+1))<='9'){
      *pos=3;
      return (((*ch)-'0')*10+(*(ch+1)-'0'));
  }

return 255;
}

unsigned char GCode::parse_g_words(unsigned char num,char * ch){
long tmp;
unsigned char tmp2=0;
char errflag=0;

point new_point;
point new_point2;

  new_point.x=0;
  new_point.y=0;  
  new_point.z=0;
  
  new_point2.x=0;
  new_point2.y=0;  
  new_point2.z=0;
  
  switch(num){
    //egyenes bejarasa leggyorsabb sebesseggel
    case 0:
      //max sebesseg beallitasa
      GCode::setspeed(GCode::max_speed);
    //egyenes bejarasa megadott sebesseggel
    case 1:
      
      //aboszolut modban ha egy koordinataertek nincs megadva, akkor nem 0 lesz a vonal kezdoerteke, hanem maradunk ott ahol vagyunk
      if(GCode::abs_mode){
        new_point.x=GCode::current_point.x;
        new_point.y=GCode::current_point.y;
        new_point.z=GCode::current_point.z;
      }
      
      //ha van X koordinataertek
      if (GCode::check_word('X',ch) ){
        tmp=GCode::get_words_nums('X',ch,&errflag);
        if (errflag) { return 1; }
        else new_point.x=tmp;
      }
      
      //ha van Y koordinataertek
      if (GCode::check_word('Y',ch) ){
        tmp=GCode::get_words_nums('Y',ch,&errflag);
        if (errflag) return 1;
        else new_point.y=tmp;
      }
      
      //ha van Z koordinataertek
      if (GCode::check_word('Z',ch) ){
        tmp=GCode::get_words_nums('Z',ch,&errflag);
        if (errflag) return 1;
        else new_point.z=tmp;
      }
      
      //ha van F feedrate
      if (GCode::check_word('F',ch) ){
        tmp=GCode::get_words_nums('F',ch,&errflag);
        if (errflag) return 1;
        else {
          GCode::feedrate=tmp;
          //feedrate szerinti sebesseg beallitasa
	  GCode::setspeed(GCode::get_speed_by_feedrate());
        }
      }
     
            
      if (GCode::abs_mode) GCode::init_line(GCode::current_point.x,GCode::current_point.y,GCode::current_point.z,new_point.x,new_point.y,new_point.z);
      else GCode::init_line(0,0,0,new_point.x,new_point.y,new_point.z);
      
      GCode::modspeed();
      
      return 0;
     
    break;
    //korinterpolacio oraval megegyezo iranyban
    case 2:
      tmp2=1;
    //korinterpolacio oraval ellentetes iranyban
    case 3:
      //ha van X koordinataertek
      if (GCode::check_word('X',ch) ){
        tmp=GCode::get_words_nums('X',ch,&errflag);
        if (errflag) return 1;
        else new_point.x=tmp;
      }
      
      //ha van Y koordinataertek
      if (GCode::check_word('Y',ch) ){
        tmp=GCode::get_words_nums('Y',ch,&errflag);
        if (errflag) return 1;
        else new_point.y=tmp;
      }
        //ha van I koordinataertek
      if (GCode::check_word('I',ch) ){
        tmp=GCode::get_words_nums('I',ch,&errflag);
        if (errflag) return 1;
        else new_point2.x=tmp;
      }
      
      //ha van J koordinataertek
      if (GCode::check_word('J',ch) ){
        tmp=GCode::get_words_nums('J',ch,&errflag);
        if (errflag) return 1;
        else new_point2.y=tmp;
      }
      
      //ha van F feedrate
      if (GCode::check_word('F',ch) ){
        tmp=GCode::get_words_nums('F',ch,&errflag);
        if (errflag) return 1;
        else {
          GCode::feedrate=tmp;
          //feedrate szerinti sebesseg beallitasa
          GCode::setspeed(GCode::get_speed_by_feedrate());
        }
      }
      
      //ha van R radius, akkora  fuggveny nem veszi figyelembe I,J  -t
      if (GCode::check_word('R',ch) ){
        tmp=GCode::get_words_nums('R',ch,&errflag);
        if (errflag) return 1;
      }
  
       if (GCode::abs_mode) {
		
		GCode::init_circle(GCode::current_point.x,GCode::current_point.y,new_point2.x,new_point2.y,new_point.x,new_point.y,tmp2,tmp);
	  }
      else {
      	GCode::init_circle(0,0,new_point2.x,new_point2.y,new_point.x,new_point.y,tmp2,tmp);
      }
      GCode::modspeed();
      
      return 0;
      
    break;
    // x-y sik
    case 17:
      GCode::plane=0;
      return 0;
    break;
    // x-z sik
    case 18:
      GCode::plane=1;
      return 0;
    break;
    // y-z sik
    case 19:
      GCode::plane=2;
      return 0;
    break;
    case 20:
      GCode::step_per_unit=GCode::step_per_inch;
      return 0;
    break;
    case 21:
      GCode::step_per_unit=GCode::step_per_mm;
      return 0;
    break;
    //absolut mode
    case 90:
      GCode::abs_mode=1;
      return 0;
    break; 
    //relativ mode
    case 91:
      GCode::abs_mode=0;
      return 0;
    break; 
    case 92:
      //alappont beallitasa
      
      new_point.x=GCode::current_point.x;
      new_point.y=GCode::current_point.y;
      new_point.z=GCode::current_point.z;
      
      //ha van X koordinataertek
      if (GCode::check_word('X',ch) ){
        tmp=GCode::get_words_nums('X',ch,&errflag);
        if (errflag) return 1;
        else new_point.x=tmp;
      }
      
      //ha van Y koordinataertek
      if (GCode::check_word('Y',ch) ){
        tmp=GCode::get_words_nums('Y',ch,&errflag);
        if (errflag) return 1;
        else new_point.y=tmp;
      }
      
      //ha van Z koordinataertek
      if (GCode::check_word('Z',ch) ){
        tmp=GCode::get_words_nums('Z',ch,&errflag);
        if (errflag) return 1;
        else new_point.z=tmp;
      }
      
      GCode::current_point.x=new_point.x;
      GCode::current_point.y=new_point.y;
      GCode::current_point.z=new_point.z;
      
      return 0;
    break; 
  }

}


unsigned char GCode::parse_m_words(unsigned char num,char * ch){
signed long tmpx,tmpy,tmpz;
char errflag=0;
  
  switch(num){
    //stop funkciok....egyenlore csak vannak....
    case 0:
    case 1:
      return 0;
    break;
    //End Program: ilyenkor modvaltas kezi modba!!
    case 2:
   
      return 0;
    break;
    //motor bekapcsolasa.....
    case 3: 
      return 0;
    break;
    
    default: 
      return 1;
    break;
  }

}

unsigned char GCode::check_word(char word,char * str){
unsigned char j=0;

    while(*(str+j)!=GCODE_COMMAND_END){
      if (GCode::toupper(*(str+j))==GCode::toupper(word)) return 1;
      j++;      
    }  
    
return 0;
}


long GCode::get_words_nums(char word,char * str,char * err){
unsigned char i=0,j=0,x,y=0;
char tmp[GCODE_MAXCMD];

//hiba flag erteke alapbol 1 lesz
(*err)=1;
 
  while(*(str+j)!=GCODE_COMMAND_END){
    
    //ha megvan a keresett szo...
    if (GCode::toupper(*(str+j))==word){

        //itt lehet whitespace
    	while(*(str+j+1)==' ' || *(str+j+1)=='\t') { ++j; }
        if (*(str+j+1)==GCODE_COMMAND_END){
            return 0;
        }

    	int i=j+1,kezd;
        
	//ha elojel akkor leptetunk
	if(*(str+i)=='+' || *(str+i)=='-') ++i;

	kezd = i;
	//ameddig szam, leptetjuk
	while(*(str+i)>='0' && *(str+i)<='9') ++i;
        //ha tul nagy a szam akkor elkuldjuk a fenebe (max 600mm lehet durvan, vagy sebessegnel ~8000max)
        //az szamjegy max
        if (kezd+6<i) { return 0; }
	//ha pont leptetunk
	if (*(str+i)=='.') ++i;
	//ameddig szam leptetunk
	while(*(str+i)>='0' && *(str+i)<='9') ++i;
        //ha nem volt szam vagy csak egyet leptunk es az egy pont volt akkor nem jo
  	if (i==kezd || i==kezd+1 && *(str+kezd)=='.') { return 0;}
	//itt mar biztosan volt egy pont es egy szam
	if (*(str+i)=='E' || *(str+i)=='e') {
		++i;
		if(*(str+i)=='+' || *(str+i)=='-') i++;
		//ha az e utan nem szam van akkor rossz
		if(*(str+i)<'0' || *(str+i)>'9') { return 0; }
		//ameg szam leptetunk
		while(*(str+i)>='0' && *(str+i)<='9') ++i;
	}
	if (*(str+i)!=' ' && *(str+i)!=GCODE_COMMAND_END && !GCode::isalpha(*(str+i))) { return 0; }
	else {
           for(x=(j+1);x<i;x++){
              tmp[y++]=*(str+x);
           }
           tmp[y]='\0';
           //Serial.println(tmp);
           //Serial.println("Szamkent...");
           //Serial.println(long(strtod(tmp,NULL)*step_per_unit),DEC);
           (*err)=0;           
		   //printf("%ld\n",long(round(strtod(tmp,NULL)*GCode::step_per_unit)));       
           return (long(round(strtod(tmp,NULL)*GCode::step_per_unit)));
        }
    }
    j++;
  }

return 0;
}

char GCode::toupper(char c){
  if (c>=97 && c<=122){
    return c-32;
  }
return c;
}

char GCode::isalpha(char c){
  if (toupper(c)>=65 && toupper(c)<=90){
    return 1;
  }
return 0;
}

/* 0 is ok, 1 can be parser error, wait to manual confirm */
unsigned char GCode::parse_command(char * cmd){
unsigned char retval;
unsigned char pos;
char errflag=0;
long tmp;

  switch(GCode::toupper(cmd[0])){
	case GCODE_COMMAND_END: //empty line
	  return 0;
	break;
    case '(': //comments
	  return 0;
	break;
	case ' ': //any empty thing
	  return 0;
	break;
    case '\t': //any empty thing
	  return 0;
	break;
  }

	
  retval=GCode::parse_typenum_to_byte((cmd)+1,&pos);
  //Serial.print(retval);
  if (retval==255) return 1;
      
  switch(GCode::toupper(cmd[0])){
    
    case 'G':
      errflag = GCode::parse_g_words(retval,(cmd)+pos);
	  if (0){
	      GCode::lastg=retval;
          }
      return errflag;
    break;
    case 'M':
      return GCode::parse_m_words(retval,(cmd)+pos);
    break;
    case 'F':
      tmp=GCode::get_words_nums('F',cmd,&errflag);
      if (errflag) return 1;
      GCode::feedrate=tmp;
      //feedrate szerinti sebesseg beallitasa
      GCode::setspeed(GCode::get_speed_by_feedrate());
      return 0;
    break;
    case 'T':
      return 1;
    break;
    //ha nincs G vagy M akkor ismetelt parancs lehet.....
    default:
      return GCode::parse_g_words(GCode::lastg,cmd);
    break;
    
  }

}

void GCode::calculate_steps(){

     switch (GCode::currcommand){
      case 1:
       GCode::calculate_line_steps();
      break;
      case 2:
       GCode::calculate_circle();
      break;
    }

    GCode::current_point.x+=GCode::x_step;
    GCode::current_point.y+=GCode::y_step;
    GCode::current_point.z+=GCode::z_step;
}

char GCode::getXstep(){
char x_step_tmp;

	x_step_tmp=GCode::x_step;
	GCode::x_step=0;
	return x_step_tmp;	
}

char GCode::getYstep(){
char y_step_tmp;

	y_step_tmp=GCode::y_step;
	GCode::y_step=0;
	return y_step_tmp;	}

char GCode::getZstep(){
char z_step_tmp;

	z_step_tmp=GCode::z_step;
	GCode::z_step=0;
	return z_step_tmp;	
}

char GCode::getmodspeed(){

    if (GCode::modspeedflag==1){ 
    	GCode::modspeedflag=0;
	return 1;
    }else{
	return 0;
    }
    
}

unsigned long GCode::getspeed(){
    return GCode::speed;	
}

char GCode::hasCommand(){
    if (currcommand!=0) return 1;
    else return 0;
}

point GCode::getCurrentPoint(){
    return GCode::current_point;
}
