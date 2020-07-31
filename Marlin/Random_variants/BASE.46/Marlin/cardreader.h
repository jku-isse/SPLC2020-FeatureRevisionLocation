
#define __CARDREADERH 


 





  
  
  
  
  
  

  
  
  
  
  
  
  
  
  
  
  
  
  

  
  
  
  


  
  
  
  

  
  
  
  
  
  
  
  

  

  


class CardReader
{
public:
  inline CardReader(){};
  
  inline static void initsd(){};
  inline static void write_command(char *buf){};
  
  inline static void checkautostart(bool x) {}; 
  
  inline static void closefile() {};
  inline static void release(){};
  inline static void startFileprint(){};
  inline static void startFilewrite(char *name){};
  inline static void pauseSDPrint(){};
  inline static void getStatus(){};
  
  inline static void selectFile(char* name){};
  inline static void getfilename(const uint8_t nr){};
  inline static uint8_t getnrfilenames(){return 0;};
  

  inline static void ls() {};
  inline static bool eof() {return true;};
  inline static char get() {return 0;};
  inline static void setIndex(){};
};

  
  
  

