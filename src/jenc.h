//jenc.h
/*
 这是一个简单的jpeg编码程序，支持1：1：1采样的baseline彩色jpeg，输入只能是24bit的BMP文件
 代码结构只求能说明各步骤过程，并不做特别的优化，效率较为一般。
*/

#ifndef __JENC__
#define __JENC__

#include <string>
#include <windows.h>
#include <stdio.h>
#include <malloc.h>
#include <math.h>
#include "jpegformat.h"
#include "jpeg.h"

using namespace std;

class JEnc
{
public:
 // bmFile:输入文件
 // jpgFile:输出文件
 // Q:质量
 void Invoke(string bmFile, string jpgFile, long Q);

private:

 FILE* pOutFile;
 int buffWidth;
 int buffHeight;
 int imgWidth;
 int imgHeight;


 // 获取BMP文件输出缓冲区信息
 BMBUFINFO GetBMBuffSize(FILE* pFile);
 
 // 获取图像数据
 void GetBMData(FILE* pFile, BYTE* pBuff, BMBUFINFO buffInfo);
 

 // 转换色彩空间BGR-YUV,111采样
 void BGR2YUV111(BYTE* pBuf, BYTE* pYBuff, BYTE* pUBuff, BYTE* pVBuff);
 

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
 void DivBuff(BYTE* pBuf,UINT width,UINT height,UINT xLen,UINT yLen);
  

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
 void SetQuantTable(const BYTE* std_QT,BYTE* QT, int Q);
 

 //为float AA&N IDCT算法初始化量化表
 void InitQTForAANDCT();
 

 //写文件开始标记
 void WriteSOI(void);

 //写APP0段
 void WriteAPP0(void);


 //写入DQT段
 void WriteDQT(void);
 

 //写入SOF段
 void WriteSOF(void);
 

 //写入DHT段
 void WriteDHT(void);
 

 //写入SOS段
 void WriteSOS(void);
 
 //写入文件结束标记
 void WriteEOI(void);
 


 // 将高8位和低8位交换
 USHORT Intel2Moto(USHORT val);
 

 //写1字节到文件
 void WriteByte(BYTE val);
 

 // 生成标准Huffman表
 void BuildSTDHuffTab(BYTE* nrcodes,BYTE* stdTab,HUFFCODE* huffCode);
 

 // 处理DU(数据单元);
 void ProcessDU(FLOAT* lpBuf,FLOAT* quantTab,HUFFCODE* dcHuffTab,HUFFCODE* acHuffTab,SHORT* DC);
 

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
 void ProcessData(BYTE* lpYBuf,BYTE* lpUBuf,BYTE* lpVBuf);
 

 // 8x8的浮点离散余弦变换
 void FDCT(FLOAT* lpBuff);
 

 //********************************************************************
 // 方法名称:WriteBits 
 //
 // 方法说明:写入二进制流
 //
 // 参数说明:
 // value:AC/DC信号的振幅
 //********************************************************************
 void WriteBits(HUFFCODE huffCode);
 
 void WriteBits(SYM2 sym);
 

 //********************************************************************
 // 方法名称:WriteBitsStream 
 //
 // 方法说明:写入二进制流
 //
 // 参数说明:
 // value:需要写入的值
 // codeLen:二进制长度
 //********************************************************************
 void WriteBitsStream(USHORT value,BYTE codeLen);
 

 //********************************************************************
 // 方法名称:RLEComp 
 //
 // 方法说明:使用RLE算法对AC压缩,假设输入数据1,0,0,0,3,0,5 
 //     输出为(0,1);(3,3);(1,5);,左位表示右位数据前0的个数
 //          左位用4bits表示,0的个数超过表示范围则输出为(15,0);
 //          其余的0数据在下一个符号中表示.
 //
 // 参数说明:
 // lpbuf:输入缓冲,8x8变换信号缓冲
 // lpOutBuf:输出缓冲,结构数组,结构信息见头文件
 // resultLen:输出缓冲长度,即编码后符号的数量
 //********************************************************************
 void RLEComp(SHORT* lpbuf,ACSYM* lpOutBuf,BYTE &resultLen);
 

 //********************************************************************
 // 方法名称:BuildSym2 
 //
 // 方法说明:将信号的振幅VLI编码,返回编码长度和信号振幅的反码
 //
 // 参数说明:
 // value:AC/DC信号的振幅
 //********************************************************************
 SYM2 BuildSym2(SHORT value);
 


 //返回符号的长度
 BYTE ComputeVLI(SHORT val);
 

 //********************************************************************
 // 方法名称:BuildVLITable 
 //
 // 方法说明:生成VLI表
 //
 // 参数说明:
 //********************************************************************
 void BuildVLITable(void);
 
};

#endif // __JENC__
