//jenc.cpp
/*
 这是一个简单的jpeg编码程序，支持1：1：1采样的baseline彩色jpeg，输入只能是24bit的BMP文件
 代码结构只求能说明各步骤过程，并不做特别的优化，效率较为一般。jpeg的色彩是采用YCrCb模式
 的，所以从BMP到jpeg要经过YUV的转换。
*/
#include "jenc.h"

// 存放VLI表
BYTE VLI_TAB[4096];

// 存放2个量化表
BYTE YQT[DCTBLOCKSIZE]; 
BYTE UVQT[DCTBLOCKSIZE]; 
// 存放2个FDCT变换要求格式的量化表
FLOAT YQT_DCT[DCTBLOCKSIZE];
FLOAT UVQT_DCT[DCTBLOCKSIZE];
//存放4个Huffman表
HUFFCODE STD_DC_Y_HT[12];
HUFFCODE STD_DC_UV_HT[12];
HUFFCODE STD_AC_Y_HT[256];
HUFFCODE STD_AC_UV_HT[256];


 // bmFile:输入文件
 // jpgFile:输出文件
 // Q:质量
void JEnc::Invoke(string bmFile, string jpgFile, long Q)
 {
  FILE* pFile;            // 输入文件句柄

  if ((pFile = fopen(bmFile.c_str(),"rb")) == NULL)   // 打开文件
  { 
   throw("open bmp file error.");   
  }

  // 获取jpeg编码需要的bmp数据结构，jpeg要求数据缓冲区的高和宽为8或16的倍数(视采样方式而定)
  BMBUFINFO bmBuffInfo = GetBMBuffSize(pFile);    
  imgWidth = bmBuffInfo.imgWidth;     // 图像宽
  imgHeight = bmBuffInfo.imgHeight;    // 图像高
  buffWidth = bmBuffInfo.buffWidth;    // 缓冲宽
  buffHeight = bmBuffInfo.buffHeight;    // 缓冲高
  size_t buffSize = buffHeight * buffWidth * 3; // 缓冲长度，因为是24bits,所以*3
  BYTE* bmData = new BYTE[buffSize];    // 申请内存空间
  GetBMData(pFile, bmData, bmBuffInfo);   // 获取数据
  fclose(pFile);         // 关闭文件

  //=====================================
  // 计算编码需要的缓冲区，RGB信号需要别分别编码，所以需要3个缓冲区，这里只是1：1：1所以是一样大
  size_t yuvBuffSize = buffWidth * buffHeight; 
  BYTE* pYBuff = new BYTE[yuvBuffSize];
  BYTE* pUBuff = new BYTE[yuvBuffSize];
  BYTE* pVBuff = new BYTE[yuvBuffSize];
  // 将RGB信号转换为YUV信号
  BGR2YUV111(bmData,pYBuff,pUBuff,pVBuff);
  // 将信号分割为8x8的块
  DivBuff(pYBuff, buffWidth, buffHeight, DCTSIZE, DCTSIZE );  
  DivBuff(pUBuff, buffWidth, buffHeight, DCTSIZE, DCTSIZE );  
  DivBuff(pVBuff, buffWidth, buffHeight, DCTSIZE, DCTSIZE );  

  SetQuantTable(std_Y_QT,YQT, Q);         // 设置Y量化表
  SetQuantTable(std_UV_QT,UVQT, Q);        // 设置UV量化表  
  InitQTForAANDCT();            // 初始化AA&N需要的量化表
  pVLITAB=VLI_TAB + 2047;                             // 设置VLI_TAB的别名
  BuildVLITable();            // 计算VLI表   

  pOutFile = fopen(jpgFile.c_str(),"wb");

  // 写入各段
  WriteSOI();              
  WriteAPP0();
  WriteDQT();
  WriteSOF();
  WriteDHT();
  WriteSOS();

  // 计算Y/UV信号的交直分量的huffman表，这里使用标准的huffman表，并不是计算得出，缺点是文件略长，但是速度快
  BuildSTDHuffTab(STD_DC_Y_NRCODES,STD_DC_Y_VALUES,STD_DC_Y_HT);
  BuildSTDHuffTab(STD_AC_Y_NRCODES,STD_AC_Y_VALUES,STD_AC_Y_HT);
  BuildSTDHuffTab(STD_DC_UV_NRCODES,STD_DC_UV_VALUES,STD_DC_UV_HT);
  BuildSTDHuffTab(STD_AC_UV_NRCODES,STD_AC_UV_VALUES,STD_AC_UV_HT);

  // 处理单元数据
  ProcessData(pYBuff,pUBuff,pVBuff);  
  WriteEOI();

  fclose(pOutFile);
  delete[] bmData;
 }

// 获取BMP文件输出缓冲区信息
BMBUFINFO JEnc::GetBMBuffSize(FILE* pFile)
 {
  BITMAPFILEHEADER bmHead;       //文件头信息块 
  BITMAPINFOHEADER bmInfo;       //图像描述信息块
  BMBUFINFO   bmBuffInfo;
  UINT colSize = 0;
  UINT rowSize = 0;

  fseek(pFile,0,SEEK_SET);       //将读写指针指向文件头部
  fread(&bmHead,sizeof(bmHead),1,pFile);    //读取文件头信息块
  fread(&bmInfo,sizeof(bmInfo),1,pFile);    //读取位图信息块

  // 计算填充后列数，jpeg编码要求缓冲区的高和宽为8或16的倍数
  if (bmInfo.biWidth % 8 == 0)
  {
   colSize = bmInfo.biWidth;
  }
  else
  {
   colSize = bmInfo.biWidth + 8 - (bmInfo.biWidth % 8);
  }

  // 计算填充后行数
  if (bmInfo.biHeight % 8 == 0)
  {
   rowSize = bmInfo.biHeight;
  }
  else
  {
   rowSize = bmInfo.biHeight + 8 - (bmInfo.biHeight % 8);
  }

  bmBuffInfo.BitCount = 24;
  bmBuffInfo.buffHeight = rowSize;   // 缓冲区高
  bmBuffInfo.buffWidth = colSize;    // 缓冲区宽
  bmBuffInfo.imgHeight = bmInfo.biHeight;  // 图像高
  bmBuffInfo.imgWidth = bmInfo.biWidth;  // 图像宽

  return bmBuffInfo;
 }

 // 获取图像数据
 void JEnc::GetBMData(FILE* pFile, BYTE* pBuff, BMBUFINFO buffInfo)
 { 
  BITMAPFILEHEADER bmHead;       // 文件头信息块 
  BITMAPINFOHEADER bmInfo;       // 图像描述信息块
  size_t    dataLen  = 0;    // 文件数据区长度
  long    alignBytes = 0;     // 为对齐4字节需要补足的字节数 
  UINT    lineSize  = 0;  

  fseek(pFile,0,SEEK_SET);       // 将读写指针指向文件头部
  fread(&bmHead,sizeof(bmHead),1,pFile);    // 读取文件头信息块
  fread(&bmInfo,sizeof(bmInfo),1,pFile);    // 读取位图信息块

  //计算对齐的字节数
  alignBytes = (((bmInfo.biWidth * bmInfo.biBitCount) + 31) & ~31) / 8L
   - (bmInfo.biWidth * bmInfo.biBitCount) / 8L; // 计算图象文件数据段行补齐字节数   

  //计算数据缓冲区长度        
  lineSize = bmInfo.biWidth * 3;      
  // 因为bmp文件数据是倒置的所以从最后一行开始读
  for (int i = bmInfo.biHeight - 1; i >= 0; --i)
  {   
   fread(&pBuff[buffInfo.buffWidth * i * 3],lineSize,1,pFile);   
   fseek(pFile,alignBytes,SEEK_CUR);             // 跳过对齐字节            
  }  
 }

 // 转换色彩空间BGR-YUV,111采样
 void JEnc::BGR2YUV111(BYTE* pBuf, BYTE* pYBuff, BYTE* pUBuff, BYTE* pVBuff)
 {
  DOUBLE tmpY   = 0;         //临时变量
  DOUBLE tmpU   = 0;
  DOUBLE tmpV   = 0;
  BYTE tmpB   = 0;       
  BYTE tmpG   = 0;
  BYTE tmpR   = 0;
  UINT i    = 0;
  size_t elemNum = _msize(pBuf) / 3;  //缓冲长度

  for (i = 0; i < elemNum; i++)
  {
   tmpB = pBuf[i * 3];
   tmpG = pBuf[i * 3 + 1];
   tmpR = pBuf[i * 3 + 2];
   tmpY = 0.299 * tmpR + 0.587 * tmpG + 0.114 * tmpB;
   tmpU = -0.1687 * tmpR - 0.3313 * tmpG + 0.5 * tmpB + 128;
   tmpV = 0.5 * tmpR - 0.4187 * tmpG - 0.0813 * tmpB + 128;
   //if(tmpY > 255){tmpY = 255;}     //输出限制
   //if(tmpU > 255){tmpU = 255;}
   //if(tmpV > 255){tmpV = 255;}
   //if(tmpY < 0){tmpY = 0;}  
   //if(tmpU < 0){tmpU = 0;}  
   //if(tmpV < 0){tmpV = 0;}
   pYBuff[i] = tmpY;           //放入输入缓冲
   pUBuff[i] = tmpU;
   pVBuff[i] = tmpV;
  }
 }

 //********************************************************************
 // 方法名称:DivBuff 
 // 最后修订日期:2003.5.3 
 //
 // 参数说明:
 // lpBuf:输入缓冲,处理后的数据也存储在这里
 // width:缓冲X方向长度
 // height:缓冲Y方向长度
 // xLen:X方向切割长度
 // yLen:Y方向切割长度
 //********************************************************************
 void JEnc::DivBuff(BYTE* pBuf,UINT width,UINT height,UINT xLen,UINT yLen)
 {
  UINT xBufs   = width / xLen;             //X轴方向上切割数量
  UINT yBufs   = height / yLen;            //Y轴方向上切割数量
  UINT tmpBufLen  = xBufs * xLen * yLen;           //计算临时缓冲区长度
  BYTE* tmpBuf  = new BYTE[tmpBufLen];           //创建临时缓冲
  UINT i    = 0;               //临时变量
  UINT j    = 0;
  UINT k    = 0; 
  UINT n    = 0;
  UINT bufOffset  = 0;               //切割开始的偏移量

  for (i = 0; i < yBufs; ++i)               //循环Y方向切割数量
  {
   n = 0;                   //复位临时缓冲区偏移量
   for (j = 0; j < xBufs; ++j)              //循环X方向切割数量  
   {   
    bufOffset = yLen * xLen * i * xBufs + j * xLen;        //计算单元信号块的首行偏移量  
    for (k = 0; k < yLen; ++k)             //循环块的行数
    {
     memcpy(&tmpBuf[n],&pBuf[bufOffset],xLen);        //复制一行到临时缓冲
     n += xLen;                //计算临时缓冲区偏移量
     bufOffset += width;              //计算输入缓冲区偏移量
    }
   }
   memcpy(&pBuf[i * tmpBufLen],tmpBuf,tmpBufLen);         //复制临时缓冲数据到输入缓冲
  } 
  delete[] tmpBuf;                 //删除临时缓冲
 } 

 //********************************************************************
 // 方法名称:SetQuantTable 
 //
 // 方法说明:根据所需质量设置量化表
 //
 // 参数说明:
 // std_QT:标准量化表
 // QT:输出量化表
 // Q:质量参数
 //********************************************************************
 // 根据所需质量设置量化表
 void JEnc::SetQuantTable(const BYTE* std_QT,BYTE* QT, int Q)
 {
  INT tmpVal = 0;                  //临时变量
  DWORD i    = 0; 

  if (Q < 1) Q = 1;               //限制质量系数
  if (Q > 100) Q = 100;

  //非线性映射 1->5000, 10->500, 25->200, 50->100, 75->50, 100->0
  if (Q < 50)
  {
   Q = 5000 / Q;
  }
  else
  {
   Q = 200 - Q * 2;
  }

  for (i = 0; i < DCTBLOCKSIZE; ++i)
  {
   tmpVal = (std_QT[i] * Q + 50L) / 100L;

   if (tmpVal < 1)                 //数值范围限定
   {
    tmpVal = 1L;
   }
   if (tmpVal > 255)
   {
    tmpVal = 255L;
   }
   QT[FZBT[i]] = static_cast<BYTE>(tmpVal);
  } 
 }

 //为float AA&N IDCT算法初始化量化表
 void JEnc::InitQTForAANDCT()
 {
  UINT i = 0;           //临时变量
  UINT j = 0;
  UINT k = 0; 

  for (i = 0; i < DCTSIZE; i++)  //初始化亮度信号量化表
  {
   for (j = 0; j < DCTSIZE; j++)
   {
    YQT_DCT[k] = (FLOAT) (1.0 / ((DOUBLE) YQT[FZBT[k]] *
     aanScaleFactor[i] * aanScaleFactor[j] * 8.0));       
    ++k;
   }
  } 

  k = 0;
  for (i = 0; i < DCTSIZE; i++)  //初始化色差信号量化表
  {
   for (j = 0; j < DCTSIZE; j++)
   {
    UVQT_DCT[k] = (FLOAT) (1.0 / ((DOUBLE) UVQT[FZBT[k]] *
     aanScaleFactor[i] * aanScaleFactor[j] * 8.0));       
    ++k;
   }
  } 
 }

 //写文件开始标记
 void JEnc::WriteSOI(void)
 { 
  fwrite(&SOITAG,sizeof(SOITAG),1,this->pOutFile);
 }
 //写APP0段
 void JEnc::WriteAPP0(void)
 {
  JPEGAPP0 APP0;
  APP0.segmentTag  = 0xE0FF;
  APP0.length    = 0x1000;
  APP0.id[0]    = 'J';
  APP0.id[1]    = 'F';
  APP0.id[2]    = 'I';
  APP0.id[3]    = 'F';
  APP0.id[4]    = 0;
  APP0.ver     = 0x0101;
  APP0.densityUnit = 0x00;
  APP0.densityX   = 0x0100;
  APP0.densityY   = 0x0100;
  APP0.thp     = 0x00;
  APP0.tvp     = 0x00;
  fwrite(&APP0,sizeof(APP0),1,this->pOutFile);
 }

 //写入DQT段
 void JEnc::WriteDQT(void)
 {
  UINT i = 0;
  JPEGDQT_8BITS DQT_Y;
  DQT_Y.segmentTag = 0xDBFF;
  DQT_Y.length   = 0x4300;
  DQT_Y.tableInfo  = 0x00;
  for (i = 0; i < DCTBLOCKSIZE; i++)
  {
   DQT_Y.table[i] = YQT[i];
  }    
  fwrite(&DQT_Y,sizeof(DQT_Y),1,this->pOutFile);

  DQT_Y.tableInfo  = 0x01;
  for (i = 0; i < DCTBLOCKSIZE; i++)
  {
   DQT_Y.table[i] = UVQT[i];
  }
  fwrite(&DQT_Y,sizeof(DQT_Y),1,this->pOutFile); 
 }

 //写入SOF段
 void JEnc::WriteSOF(void)
 {
  JPEGSOF0_24BITS SOF;
  SOF.segmentTag = 0xC0FF;
  SOF.length   = 0x1100;
  SOF.precision  = 0x08;
  SOF.height   = Intel2Moto(USHORT(this->imgHeight));
  SOF.width    = Intel2Moto(USHORT(this->imgWidth)); 
  SOF.sigNum   = 0x03;
  SOF.YID     = 0x01; 
  SOF.QTY     = 0x00;
  SOF.UID     = 0x02;
  SOF.QTU     = 0x01;
  SOF.VID     = 0x03;
  SOF.QTV     = 0x01;
  SOF.HVU     = 0x11;
  SOF.HVV     = 0x11;
  /*switch (this->SamplingType)
  {
  case 1:
  SOF.HVY   = 0x11;
  break;

  case 2:
  SOF.HVY   = 0x12;
  break;

  case 3:
  SOF.HVY   = 0x21;
  break;

  case 4:
  SOF.HVY   = 0x22;
  break;
  }*/
  SOF.HVY   = 0x11;
  fwrite(&SOF,sizeof(SOF),1,this->pOutFile);
 }

 //写入DHT段
 void JEnc::WriteDHT(void)
 {
  UINT i = 0;

  JPEGDHT DHT;
  DHT.segmentTag = 0xC4FF;
  DHT.length   = Intel2Moto(19 + 12);
  DHT.tableInfo  = 0x00;
  for (i = 0; i < 16; i++)
  {
   DHT.huffCode[i] = STD_DC_Y_NRCODES[i + 1];
  } 
  fwrite(&DHT,sizeof(DHT),1,this->pOutFile);
  for (i = 0; i <= 11; i++)
  {
   WriteByte(STD_DC_Y_VALUES[i]);  
  }  
  //------------------------------------------------
  DHT.tableInfo  = 0x01;
  for (i = 0; i < 16; i++)
  {
   DHT.huffCode[i] = STD_DC_UV_NRCODES[i + 1];
  }
  fwrite(&DHT,sizeof(DHT),1,this->pOutFile);
  for (i = 0; i <= 11; i++)
  {
   WriteByte(STD_DC_UV_VALUES[i]);  
  } 
  //----------------------------------------------------
  DHT.length   = Intel2Moto(19 + 162);
  DHT.tableInfo  = 0x10;
  for (i = 0; i < 16; i++)
  {
   DHT.huffCode[i] = STD_AC_Y_NRCODES[i + 1];
  }
  fwrite(&DHT,sizeof(DHT),1,this->pOutFile);
  for (i = 0; i <= 161; i++)
  {
   WriteByte(STD_AC_Y_VALUES[i]);  
  }  
  //-----------------------------------------------------
  DHT.tableInfo  = 0x11;
  for (i = 0; i < 16; i++)
  {
   DHT.huffCode[i] = STD_AC_UV_NRCODES[i + 1];
  }
  fwrite(&DHT,sizeof(DHT),1,this->pOutFile); 
  for (i = 0; i <= 161; i++)
  {
   WriteByte(STD_AC_UV_VALUES[i]);  
  } 
 }

 //写入SOS段
 void JEnc::WriteSOS(void)
 {
  JPEGSOS_24BITS SOS;
  SOS.segmentTag   = 0xDAFF;
  SOS.length    = 0x0C00;
  SOS.sigNum    = 0x03;
  SOS.YID     = 0x01;
  SOS.HTY     = 0x00;
  SOS.UID     = 0x02;
  SOS.HTU     = 0x11;
  SOS.VID     = 0x03;
  SOS.HTV     = 0x11;
  SOS.Se     = 0x3F;
  SOS.Ss     = 0x00;
  SOS.Bf     = 0x00;
  fwrite(&SOS,sizeof(SOS),1,this->pOutFile); 
 }
 //写入文件结束标记
 void JEnc::WriteEOI(void)
 {
  fwrite(&EOITAG,sizeof(EOITAG),1,this->pOutFile);
 }


 // 将高8位和低8位交换
 USHORT JEnc::Intel2Moto(USHORT val)
 {
  BYTE highBits = BYTE(val / 256);
  BYTE lowBits = BYTE(val % 256);

  return lowBits * 256 + highBits;
 }

 //写1字节到文件
 void JEnc::WriteByte(BYTE val)
 {   
  fwrite(&val,sizeof(val),1,this->pOutFile);
 }

 // 生成标准Huffman表
 void JEnc::BuildSTDHuffTab(BYTE* nrcodes,BYTE* stdTab,HUFFCODE* huffCode)
 {
  BYTE i     = 0;             //临时变量
  BYTE j     = 0;
  BYTE k     = 0;
  USHORT code   = 0; 

  for (i = 1; i <= 16; i++)
  { 
   for (j = 1; j <= nrcodes[i]; j++)
   {   
    huffCode[stdTab[k]].code = code;
    huffCode[stdTab[k]].length = i;
    ++k;
    ++code;
   }
   code*=2;
  } 

  for (i = 0; i < k; i++)
  {
   huffCode[i].val = stdTab[i];  
  }
 }

 // 处理DU(数据单元)
 void JEnc::ProcessDU(FLOAT* lpBuf,FLOAT* quantTab,HUFFCODE* dcHuffTab,HUFFCODE* acHuffTab,SHORT* DC)
 {
  BYTE i    = 0;              //临时变量
  UINT j    = 0;
  SHORT diffVal = 0;                //DC差异值  
  BYTE acLen  = 0;               //熵编码后AC中间符号的数量
  SHORT sigBuf[DCTBLOCKSIZE];              //量化后信号缓冲
  ACSYM acSym[DCTBLOCKSIZE];              //AC中间符号缓冲 

  FDCT(lpBuf);                 //离散余弦变换

  for (i = 0; i < DCTBLOCKSIZE; i++)            //量化操作
  {          
   sigBuf[FZBT[i]] = (lpBuf[i] * quantTab[i] + 16384.5) - 16384;  
  }
  //-----------------------------------------------------
  //对DC信号编码，写入文件
  //DPCM编码 
  diffVal = sigBuf[0] - *DC;
  *DC = sigBuf[0];
  //搜索Huffman表，写入相应的码字
  if (diffVal == 0)
  {  
   WriteBits(dcHuffTab[0]);  
  }
  else
  {   
   WriteBits(dcHuffTab[pVLITAB[diffVal]]);  
   WriteBits(BuildSym2(diffVal));    
  }
  //-------------------------------------------------------
  //对AC信号编码并写入文件
  for (i = 63; (i > 0) && (sigBuf[i] == 0); i--) //判断ac信号是否全为0
  {
   //注意，空循环
  }
  if (i == 0)                //如果全为0
  {
   WriteBits(acHuffTab[0x00]);           //写入块结束标记  
  }
  else
  { 
   RLEComp(sigBuf,&acSym[0],acLen);         //对AC运行长度编码 
   for (j = 0; j < acLen; j++)           //依次对AC中间符号Huffman编码
   {   
    if (acSym[j].codeLen == 0)          //是否有连续16个0
    {   
     WriteBits(acHuffTab[0xF0]);         //写入(15,0)    
    }
    else
    {
     WriteBits(acHuffTab[acSym[j].zeroLen * 16 + acSym[j].codeLen]); //
     WriteBits(BuildSym2(acSym[j].amplitude));    
    }   
   }
   if (i != 63)              //如果最后位以0结束就写入EOB
   {
    WriteBits(acHuffTab[0x00]);          
   }
  }
 }

 //********************************************************************
 // 方法名称:ProcessData 
 //
 // 方法说明:处理图像数据FDCT-QUANT-HUFFMAN
 //
 // 参数说明:
 // lpYBuf:亮度Y信号输入缓冲
 // lpUBuf:色差U信号输入缓冲
 // lpVBuf:色差V信号输入缓冲
 //********************************************************************
 void JEnc::ProcessData(BYTE* lpYBuf,BYTE* lpUBuf,BYTE* lpVBuf)
 { 
  size_t yBufLen = _msize(lpYBuf);           //亮度Y缓冲长度
  size_t uBufLen = _msize(lpUBuf);           //色差U缓冲长度          
  size_t vBufLen = _msize(lpVBuf);           //色差V缓冲长度
  FLOAT dctYBuf[DCTBLOCKSIZE];            //Y信号FDCT编码临时缓冲
  FLOAT dctUBuf[DCTBLOCKSIZE];            //U信号FDCT编码临时缓冲 
  FLOAT dctVBuf[DCTBLOCKSIZE];            //V信号FDCT编码临时缓冲 
  UINT mcuNum   = 0;             //存放MCU的数量 
  SHORT yDC   = 0;             //Y信号的当前块的DC
  SHORT uDC   = 0;             //U信号的当前块的DC
  SHORT vDC   = 0;             //V信号的当前块的DC 
  BYTE yCounter  = 0;             //YUV信号各自的写入计数器
  BYTE uCounter  = 0;
  BYTE vCounter  = 0;
  UINT i    = 0;             //临时变量              
  UINT j    = 0;                 
  UINT k    = 0;
  UINT p    = 0;
  UINT m    = 0;
  UINT n    = 0;
  UINT s    = 0; 

  mcuNum = (this->buffHeight * this->buffWidth * 3)
   / (DCTBLOCKSIZE * 3);         //计算MCU的数量

  for (p = 0;p < mcuNum; p++)        //依次生成MCU并写入
  {
   yCounter = 1;//MCUIndex[SamplingType][0];   //按采样方式初始化各信号计数器
   uCounter = 1;//MCUIndex[SamplingType][1];
   vCounter = 1;//MCUIndex[SamplingType][2];

   for (; i < yBufLen; i += DCTBLOCKSIZE)
   {
    for (j = 0; j < DCTBLOCKSIZE; j++)
    {
     dctYBuf[j] = FLOAT(lpYBuf[i + j] - 128);
    }   
    if (yCounter > 0)
    {    
     --yCounter;
     ProcessDU(dctYBuf,YQT_DCT,STD_DC_Y_HT,STD_AC_Y_HT,&yDC);     
    }
    else
    {
     break;
    }
   }  
   //------------------------------------------------------------------  
   for (; m < uBufLen; m += DCTBLOCKSIZE)
   {
    for (n = 0; n < DCTBLOCKSIZE; n++)
    {
     dctUBuf[n] = FLOAT(lpUBuf[m + n] - 128);
    }    
    if (uCounter > 0)
    {    
     --uCounter;
     ProcessDU(dctUBuf,UVQT_DCT,STD_DC_UV_HT,STD_AC_UV_HT,&uDC);         
    }
    else
    {
     break;
    }
   }  
   //-------------------------------------------------------------------  
   for (; s < vBufLen; s += DCTBLOCKSIZE)
   {
    for (k = 0; k < DCTBLOCKSIZE; k++)
    {
     dctVBuf[k] = FLOAT(lpVBuf[s + k] - 128);
    }
    if (vCounter > 0)
    {
     --vCounter;
     ProcessDU(dctVBuf,UVQT_DCT,STD_DC_UV_HT,STD_AC_UV_HT,&vDC);        
    }
    else
    {
     break;
    }
   }  
  } 
 }

 // 8x8的浮点离散余弦变换
 void JEnc::FDCT(FLOAT* lpBuff)
 {
  FLOAT tmp0, tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7;
  FLOAT tmp10, tmp11, tmp12, tmp13;
  FLOAT z1, z2, z3, z4, z5, z11, z13;
  FLOAT* dataptr;
  int ctr;

  /* 第一部分，对行进行计算 */
  dataptr = lpBuff;
  for (ctr = DCTSIZE-1; ctr >= 0; ctr--)
  {
   tmp0 = dataptr[0] + dataptr[7];
   tmp7 = dataptr[0] - dataptr[7];
   tmp1 = dataptr[1] + dataptr[6];
   tmp6 = dataptr[1] - dataptr[6];
   tmp2 = dataptr[2] + dataptr[5];
   tmp5 = dataptr[2] - dataptr[5];
   tmp3 = dataptr[3] + dataptr[4];
   tmp4 = dataptr[3] - dataptr[4];

   /* 对偶数项进行运算 */    
   tmp10 = tmp0 + tmp3; /* phase 2 */
   tmp13 = tmp0 - tmp3;
   tmp11 = tmp1 + tmp2;
   tmp12 = tmp1 - tmp2;

   dataptr[0] = tmp10 + tmp11; /* phase 3 */
   dataptr[4] = tmp10 - tmp11;

   z1 = (tmp12 + tmp13) * (0.707106781); /* c4 */
   dataptr[2] = tmp13 + z1; /* phase 5 */
   dataptr[6] = tmp13 - z1;

   /* 对奇数项进行计算 */
   tmp10 = tmp4 + tmp5; /* phase 2 */
   tmp11 = tmp5 + tmp6;
   tmp12 = tmp6 + tmp7;

   z5 = (tmp10 - tmp12) * ( 0.382683433); /* c6 */
   z2 = (0.541196100) * tmp10 + z5; /* c2-c6 */
   z4 = (1.306562965) * tmp12 + z5; /* c2+c6 */
   z3 = tmp11 * (0.707106781); /* c4 */

   z11 = tmp7 + z3;  /* phase 5 */
   z13 = tmp7 - z3;

   dataptr[5] = z13 + z2; /* phase 6 */
   dataptr[3] = z13 - z2;
   dataptr[1] = z11 + z4;
   dataptr[7] = z11 - z4;

   dataptr += DCTSIZE; /* 将指针指向下一行 */
  }

  /* 第二部分，对列进行计算 */
  dataptr = lpBuff;
  for (ctr = DCTSIZE-1; ctr >= 0; ctr--)
  {
   tmp0 = dataptr[DCTSIZE*0] + dataptr[DCTSIZE*7];
   tmp7 = dataptr[DCTSIZE*0] - dataptr[DCTSIZE*7];
   tmp1 = dataptr[DCTSIZE*1] + dataptr[DCTSIZE*6];
   tmp6 = dataptr[DCTSIZE*1] - dataptr[DCTSIZE*6];
   tmp2 = dataptr[DCTSIZE*2] + dataptr[DCTSIZE*5];
   tmp5 = dataptr[DCTSIZE*2] - dataptr[DCTSIZE*5];
   tmp3 = dataptr[DCTSIZE*3] + dataptr[DCTSIZE*4];
   tmp4 = dataptr[DCTSIZE*3] - dataptr[DCTSIZE*4];

   /* 对偶数项进行运算 */    
   tmp10 = tmp0 + tmp3; /* phase 2 */
   tmp13 = tmp0 - tmp3;
   tmp11 = tmp1 + tmp2;
   tmp12 = tmp1 - tmp2;

   dataptr[DCTSIZE*0] = tmp10 + tmp11; /* phase 3 */
   dataptr[DCTSIZE*4] = tmp10 - tmp11;

   z1 = (tmp12 + tmp13) * (0.707106781); /* c4 */
   dataptr[DCTSIZE*2] = tmp13 + z1; /* phase 5 */
   dataptr[DCTSIZE*6] = tmp13 - z1;

   /* 对奇数项进行计算 */
   tmp10 = tmp4 + tmp5; /* phase 2 */
   tmp11 = tmp5 + tmp6;
   tmp12 = tmp6 + tmp7;

   z5 = (tmp10 - tmp12) * (0.382683433); /* c6 */
   z2 = (0.541196100) * tmp10 + z5; /* c2-c6 */
   z4 = (1.306562965) * tmp12 + z5; /* c2+c6 */
   z3 = tmp11 * (0.707106781); /* c4 */

   z11 = tmp7 + z3;  /* phase 5 */
   z13 = tmp7 - z3;

   dataptr[DCTSIZE*5] = z13 + z2; /* phase 6 */
   dataptr[DCTSIZE*3] = z13 - z2;
   dataptr[DCTSIZE*1] = z11 + z4;
   dataptr[DCTSIZE*7] = z11 - z4;

   ++dataptr;   /* 将指针指向下一列 */
  }
 }

 //********************************************************************
 // 方法名称:WriteBits 
 //
 // 方法说明:写入二进制流
 //
 // 参数说明:
 // value:AC/DC信号的振幅
 //********************************************************************
 void JEnc::WriteBits(HUFFCODE huffCode)
 {  
  WriteBitsStream(huffCode.code,huffCode.length); 
 }
 void JEnc::WriteBits(SYM2 sym)
 {
  WriteBitsStream(sym.amplitude,sym.codeLen); 
 }

 //********************************************************************
 // 方法名称:WriteBitsStream 
 //
 // 方法说明:写入二进制流
 //
 // 参数说明:
 // value:需要写入的值
 // codeLen:二进制长度
 //********************************************************************
 void JEnc::WriteBitsStream(USHORT value,BYTE codeLen)
 { 
  CHAR posval;//bit position in the bitstring we read, should be<=15 and >=0 
  posval=codeLen-1;
  while (posval>=0)
  {
   if (value & mask[posval])
   {
    bytenew|=mask[bytepos];
   }
   posval--;bytepos--;
   if (bytepos<0) 
   { 
    if (bytenew==0xFF)
    {
     WriteByte(0xFF);
     WriteByte(0);
    }
    else
    {
     WriteByte(bytenew);
    }
    bytepos=7;bytenew=0;
   }
  }
 }

 //********************************************************************
 // 方法名称:RLEComp 
 //
 // 方法说明:使用RLE算法对AC压缩,假设输入数据1,0,0,0,3,0,5 
 //     输出为(0,1)(3,3)(1,5),左位表示右位数据前0的个数
 //          左位用4bits表示,0的个数超过表示范围则输出为(15,0)
 //          其余的0数据在下一个符号中表示.
 //
 // 参数说明:
 // lpbuf:输入缓冲,8x8变换信号缓冲
 // lpOutBuf:输出缓冲,结构数组,结构信息见头文件
 // resultLen:输出缓冲长度,即编码后符号的数量
 //********************************************************************
 void JEnc::RLEComp(SHORT* lpbuf,ACSYM* lpOutBuf,BYTE &resultLen)
 {  
  BYTE zeroNum     = 0;       //0行程计数器
  UINT EOBPos      = 0;       //EOB出现位置 
  const BYTE MAXZEROLEN = 15;          //最大0行程
  UINT i        = 0;      //临时变量
  UINT j        = 0;

  EOBPos = DCTBLOCKSIZE - 1;          //设置起始位置，从最后一个信号开始
  for (i = EOBPos; i > 0; i--)         //从最后的AC信号数0的个数
  {
   if (lpbuf[i] == 0)           //判断数据是否为0
   {
    --EOBPos;            //向前一位
   }
   else              //遇到非0，跳出
   {
    break;                   
   }
  }

  for (i = 1; i <= EOBPos; i++)         //从第二个信号，即AC信号开始编码
  {
   if (lpbuf[i] == 0 && zeroNum < MAXZEROLEN)     //如果信号为0并连续长度小于15
   {
    ++zeroNum;   
   }
   else
   {   
    lpOutBuf[j].zeroLen = zeroNum;       //0行程（连续长度）
    lpOutBuf[j].codeLen = ComputeVLI(lpbuf[i]);    //幅度编码长度
    lpOutBuf[j].amplitude = lpbuf[i];      //振幅      
    zeroNum = 0;           //0计数器复位
    ++resultLen;           //符号数量++
    ++j;             //符号计数
   }
  } 
 }

 //********************************************************************
 // 方法名称:BuildSym2 
 //
 // 方法说明:将信号的振幅VLI编码,返回编码长度和信号振幅的反码
 //
 // 参数说明:
 // value:AC/DC信号的振幅
 //********************************************************************
 SYM2 JEnc::BuildSym2(SHORT value)
 {
  SYM2 Symbol;  

  Symbol.codeLen = ComputeVLI(value);              //获取编码长度
  Symbol.amplitude = 0;
  if (value >= 0)
  {
   Symbol.amplitude = value;
  }
  else
  {
   Symbol.amplitude = SHORT(pow(2,Symbol.codeLen)-1) + value;  //计算反码
  }

  return Symbol;
 }


 //返回符号的长度
 BYTE JEnc::ComputeVLI(SHORT val)
 { 
  BYTE binStrLen = 0;
  val = abs(val); 
  //获取二进制码长度   
  if(val == 1)
  {
   binStrLen = 1;  
  }
  else if(val >= 2 && val <= 3)
  {
   binStrLen = 2;
  }
  else if(val >= 4 && val <= 7)
  {
   binStrLen = 3;
  }
  else if(val >= 8 && val <= 15)
  {
   binStrLen = 4;
  }
  else if(val >= 16 && val <= 31)
  {
   binStrLen = 5;
  }
  else if(val >= 32 && val <= 63)
  {
   binStrLen = 6;
  }
  else if(val >= 64 && val <= 127)
  {
   binStrLen = 7;
  }
  else if(val >= 128 && val <= 255)
  {
   binStrLen = 8;
  }
  else if(val >= 256 && val <= 511)
  {
   binStrLen = 9;
  }
  else if(val >= 512 && val <= 1023)
  {
   binStrLen = 10;
  }
  else if(val >= 1024 && val <= 2047)
  {
   binStrLen = 11;
  }

  return binStrLen;
 }

 //********************************************************************
 // 方法名称:BuildVLITable 
 //
 // 方法说明:生成VLI表
 //
 // 参数说明:
 //********************************************************************
 void JEnc::BuildVLITable(void)
 {
  int i   = 0;

  for (i = 0; i < DC_MAX_QUANTED; ++i)
  {
   pVLITAB[i] = ComputeVLI(i);
  }

  for (i = DC_MIN_QUANTED; i < 0; ++i)
  {
   pVLITAB[i] = ComputeVLI(i);
  }
 }
