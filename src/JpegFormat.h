//Jpegformat.h

#ifndef __JPEGFORMAT__H__
#define __JPEGFORMAT__H__

//文件开始,开始标记为0xFFD8
const static WORD SOITAG = 0xD8FF;

//文件结束,结束标记为0xFFD9
const static WORD EOITAG = 0xD9FF;

//JFIF APP0段结构
#pragma pack(push,1)
typedef struct tagJPEGAPP0
{
 WORD segmentTag;  //APP0段标记，必须为FFE0
 WORD length;    //段长度，一般为16，如果没有缩略图
 CHAR id[5];     //文件标记 "JFIF" + "\0"
 WORD ver;      //文件版本，一般为0101或0102
 BYTE densityUnit; //密度单位，0=无单位 1=点数/英寸 2=点数/厘米
 WORD densityX;   //X轴方向密度,通常写1
 WORD densityY;   //Y轴方向密度,通常写1
 BYTE thp;     //缩略图水平像素数,写0
 BYTE tvp;     //缩略图垂直像素数,写0
}JPEGAPP0;// = {0xE0FF,16,'J','F','I','F',0,0x0101,0,1,1,0,0};
#pragma pack(pop)

//JFIF APPN段结构
#pragma pack(push,1)
typedef struct tagJPEGAPPN
{
 WORD segmentTag;  //APPn段标记，从FFE0 - FFEF n=0-F
 WORD length;    //段长度   
}JPEGAPPN;
#pragma pack(pop)  

//JFIF DQT段结构(8 bits 量化表)
#pragma pack(push,1)
typedef struct tagJPEGDQT_8BITS
{
 WORD segmentTag;  //DQT段标记，必须为0xFFDB
 WORD length;    //段长度,这里是0x4300
 BYTE tableInfo;  //量化表信息
 BYTE table[64];  //量化表(8 bits)
}JPEGDQT_8BITS;
#pragma pack(pop)

//JFIF DQT段结构(8 bits 量化表)
#pragma pack(push,1)
typedef struct tagJPEGDQT_16BITS
{
 WORD segmentTag;  //DQT段标记，必须为0xFFDB
 WORD length;    //段长度，这里是0x8300
 BYTE tableInfo;  //量化表信息
 WORD table[64];   //量化表(16 bits)
}JPEGDQT_16BITS;
#pragma pack(pop)

//JFIF SOF0段结构(真彩)，其余还有SOF1-SOFF
#pragma pack(push,1)
typedef struct tagJPEGSOF0_24BITS
{
 WORD segmentTag;  //SOF段标记，必须为0xFFC0
 WORD length;    //段长度，真彩图为17，灰度图为11
 BYTE precision;  //精度，每个信号分量所用的位数，基本系统为0x08
 WORD height;    //图像高度
 WORD width;     //图像宽度
 BYTE sigNum;   //信号数量，真彩JPEG应该为3，灰度为1
 BYTE YID;     //信号编号，亮度Y
 BYTE HVY;     //采样方式，0-3位是垂直采样，4-7位是水平采样
 BYTE QTY;     //对应量化表号
 BYTE UID;     //信号编号，色差U
 BYTE HVU;     //采样方式，0-3位是垂直采样，4-7位是水平采样
 BYTE QTU;     //对应量化表号
 BYTE VID;     //信号编号，色差V
 BYTE HVV;     //采样方式，0-3位是垂直采样，4-7位是水平采样
 BYTE QTV;     //对应量化表号
}JPEGSOF0_24BITS;// = {0xC0FF,0x0011,8,0,0,3,1,0x11,0,2,0x11,1,3,0x11,1};
#pragma pack(pop)

//JFIF SOF0段结构(灰度)，其余还有SOF1-SOFF
#pragma pack(push,1)
typedef struct tagJPEGSOF0_8BITS
{
 WORD segmentTag;  //SOF段标记，必须为0xFFC0
 WORD length;    //段长度，真彩图为17，灰度图为11
 BYTE precision;  //精度，每个信号分量所用的位数，基本系统为0x08
 WORD height;    //图像高度
 WORD width;     //图像宽度
 BYTE sigNum;   //信号数量，真彩JPEG应该为3，灰度为1
 BYTE YID;     //信号编号，亮度Y
 BYTE HVY;     //采样方式，0-3位是垂直采样，4-7位是水平采样
 BYTE QTY;     //对应量化表号 
}JPEGSOF0_8BITS;// = {0xC0FF,0x000B,8,0,0,1,1,0x11,0};
#pragma pack(pop)

//JFIF DHT段结构
#pragma pack(push,1)
typedef struct tagJPEGDHT
{
 WORD segmentTag;  //DHT段标记，必须为0xFFC4
 WORD length;    //段长度
 BYTE tableInfo;  //表信息，基本系统中 bit0-3 为Huffman表的数量，bit4 为0指DC的Huffman表 为1指AC的Huffman表，bit5-7保留，必须为0
 BYTE huffCode[16];//1-16位的Huffman码字的数量，分别存放在数组[1-16]中
 //BYTE* huffVal;  //依次存放各码字对应的值
}JPEGDHT;
#pragma pack(pop)

// JFIF SOS段结构（真彩）
#pragma pack(push,1)
typedef struct tagJPEGSOS_24BITS
{
 WORD segmentTag;  //SOS段标记，必须为0xFFDA
 WORD length;    //段长度，这里是12
 BYTE sigNum;   //信号分量数，真彩图为0x03,灰度图为0x01
 BYTE YID;     //亮度Y信号ID,这里是1
 BYTE HTY;     //Huffman表号，bit0-3为DC信号的表，bit4-7为AC信号的表
 BYTE UID;     //亮度Y信号ID,这里是2
 BYTE HTU;
 BYTE VID;     //亮度Y信号ID,这里是3
 BYTE HTV;
 BYTE Ss;     //基本系统中为0
 BYTE Se;     //基本系统中为63
 BYTE Bf;     //基本系统中为0
}JPEGSOS_24BITS;// = {0xDAFF,0x000C,3,1,0,2,0x11,3,0x11,0,0x3F,0};
#pragma pack(pop)

// JFIF SOS段结构（灰度）
#pragma pack(push,1)
typedef struct tagJPEGSOS_8BITS
{
 WORD segmentTag;  //SOS段标记，必须为0xFFDA
 WORD length;    //段长度，这里是8
 BYTE sigNum;   //信号分量数，真彩图为0x03,灰度图为0x01
 BYTE YID;     //亮度Y信号ID,这里是1
 BYTE HTY;     //Huffman表号，bit0-3为DC信号的表，bit4-7为AC信号的表  
 BYTE Ss;     //基本系统中为0
 BYTE Se;     //基本系统中为63
 BYTE Bf;     //基本系统中为0
}JPEGSOS_8BITS;// = {0xDAFF,0x0008,1,1,0,0,0x3F,0};
#pragma pack(pop) 

// JFIF COM段结构
#pragma pack(push,1)
typedef struct tagJPEGCOM
{
 WORD segmentTag;  //COM段标记，必须为0xFFFE
 WORD length;    //注释长度
}JPEGCOM;
#pragma pack(pop) 

#endif //__JPEGFORMAT__H__
