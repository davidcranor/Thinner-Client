
/*
 * makeFontFile
 * 
 * Converts a 256 character .png font into a format suitable for thinnerclient.
 *
 * David Cranor
 * 8/30/10
 *
 * (c) Massachusetts Institute of Technology 2010
 * Permission granted for experimental and personal use;
 * license for commercial sale available from MIT.
 */

 import java.awt.Color;

void setup()
{

  size(200, 200);

}

void draw()
{

  PImage inFont;
  inFont = loadImage("inFont.png");
  PrintWriter fontFile;

  fontFile = createWriter("Font6x8.c");
  fontFile.println("#include \"stm32f10x.h\"");
  fontFile.println("uint8_t Font6x8[256][8] = {");
  fontFile.println("{");

  for(int i=0; i < 256; i++)
  {
    fontFile.println();

    for(int j=0; j < 8; j++)
    {

      String fontLine = "0b";

      for(int k=0; k < 6; k++)
      {

        Color c = new Color(inFont.pixels[i*6+j*(256*6)+k]);
        
        if(c.getRGB() == #000000)
        {

          fontLine = fontLine + "0";  

        } 
        else {

          fontLine = fontLine + "1";  

        }  

      }

      fontLine = fontLine + "00";
      
      if(j < 7 )
      {
        
        fontLine = fontLine + ",";
        
      }
      
      if(j == 0)
      {
        
        fontLine = fontLine + "    //Character " + i;
        
      }
      
      
      fontFile.println(fontLine);


    }

  fontFile.println();  
  fontFile.println("}, {");

  }

  fontFile.println("};");

  fontFile.flush(); // Writes the remaining data to the file
  fontFile.close(); // Finishes the file
  exit();
}


