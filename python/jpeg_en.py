# -*- coding: utf-8 -*-
"""
Created on Sat Jan 23 20:35:23 2018

@author: wang

@function: basic JPEG encoder. DCT + QT +Zigzag + VLC
"""

import img_op
import math
import cv2

'''
Based on itu-t81 AnnexK.1
'''

DCTSIZE = 8
DCTBLOCKSIZE = DCTSIZE*DCTSIZE

DC_MAX_QUANTED = 2047
DC_MIN_QUANTED = -2048

STD_Y_QT = (16, 11, 10, 16, 24, 40, 51, 61,
            12, 12, 14, 19, 26, 58, 60, 55,
            14, 13, 16, 24, 40, 57, 69, 56,
            14, 17, 22, 29, 51, 87, 80, 62,
            18, 22, 37, 56, 68, 109, 103, 77,
            24, 35, 55, 64, 81, 104, 113, 92,
            49, 64, 78, 87, 103, 121, 120, 101,
            72, 92, 95, 98, 112, 100, 103, 99)

STD_UV_QT = (17,18,24,47,99,99,99,99,
             18,21,26,66,99,99,99,99,
             24,26,56,99,99,99,99,99,
             47,66,99,99,99,99,99,99,
             99,99,99,99,99,99,99,99,
             99,99,99,99,99,99,99,99,
             99,99,99,99,99,99,99,99,
             99,99,99,99,99,99,99,99)

STD_ZIGZAGT = (0,1,5,6,14,15,27,28,
               2,4,7,13,16,26,29,42,
               3,8,12,17,25,30,41,43,
               9,11,18,24,31,40,44,53,
               10,19,23,32,39,45,52,54,
               20,22,33,38,46,51,55,60,
               21,34,37,47,50,56,59,61,
               35,36,48,49,57,58,62,63)

STD_Y_DC_HUFSIZE = (0,1,5,1,1,1,1,1,1,0,0,0,0,0,0,0)
STD_Y_DC_HUFVALU = (0,1,2,3,4,5,6,7,8,9,10,11)
STD_UV_DC_HUFSIZE = (0,3,1,1,1,1,1,1,1,1,1,0,0,0,0,0)
STD_UV_DC_HUFVALU = (0,1,2,3,4,5,6,7,8,9,10,11)
STD_Y_AC_HUFSIZE = (0,2,1,3,3,2,4,3,5,5,4,4,0,0,1,0x7d)
STD_Y_AC_HUFVALU = (0x01, 0x02, 0x03, 0x00, 0x04, 0x11, 0x05, 0x12, 0x21, 0x31, 0x41, 0x06, 0x13, 0x51, 0x61, 0x07,
                    0x22, 0x71, 0x14, 0x32, 0x81, 0x91, 0xa1, 0x08, 0x23, 0x42, 0xb1, 0xc1, 0x15, 0x52, 0xd1, 0xf0,
                    0x24, 0x33, 0x62, 0x72, 0x82, 0x09, 0x0a, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x25, 0x26, 0x27, 0x28,
                    0x29, 0x2a, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49,
                    0x4a, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5a, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69,
                    0x6a, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7a, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89,
                    0x8a, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9a, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7,
                    0xa8, 0xa9, 0xaa, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xc2, 0xc3, 0xc4, 0xc5,
                    0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda, 0xe1, 0xe2,
                    0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8,
                    0xf9, 0xfa)
STD_UV_AC_HUFSIZE = (0,2,1,2,4,4,3,4,7,5,4,4,0,1,2,0x77)
STD_UV_AC_HUFVALU = (0x00, 0x01, 0x02, 0x03, 0x11, 0x04, 0x05, 0x21, 0x31, 0x06, 0x12, 0x41, 0x51, 0x07, 0x61, 0x71,
                     0x13, 0x22, 0x32, 0x81, 0x08, 0x14, 0x42, 0x91, 0xa1, 0xb1, 0xc1, 0x09, 0x23, 0x33, 0x52, 0xf0,
                     0x15, 0x62, 0x72, 0xd1, 0x0a, 0x16, 0x24, 0x34, 0xe1, 0x25, 0xf1, 0x17, 0x18, 0x19, 0x1a, 0x26,
                     0x27, 0x28, 0x29, 0x2a, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48,
                     0x49, 0x4a, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5a, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68,
                     0x69, 0x6a, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7a, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
                     0x88, 0x89, 0x8a, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9a, 0xa2, 0xa3, 0xa4, 0xa5,
                     0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xc2, 0xc3,
                     0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda,
                     0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8,
                     0xf9, 0xfa)

'''
方法名称： div_buff
功能描述： 将连续存储图像切割成block方式存储(ITU-T81 A.2.2)
参数说明：
    inbuff -- 图像buff
    hsize  -- 图像hsize, 应该是8的倍数
    vsize  -- 图像vsize, 应该是8的倍数
    xlen   -- x方向图像切割长度, 应该是8
    ylen   -- y方向图像切割长度, 应该是8
返回值  ： 切割成block存储的图像buff
'''
def div_buff(inbuff, hsize, vsize, xlen, ylen):
    div_buff = []
    for j in range(vsize/ylen):
        for i in range(hsize/xlen):
            for k in range(xlen*ylen):
                src_addr = j*hsize*ylen + i*xlen + (k % xlen) + (k/xlen)*hsize
                div_buff.append(inbuff[src_addr])
    return div_buff            

'''
方法名称： fdct
功能描述： 前向离散余弦变换(ITU-T81 A.3.3)
参数说明：
    block -- 输入8x8block图像
返回值  ： dct变换结果， 8x8 block
'''
def fdct(block):
    dctblock = []
    for u in range(DCTSIZE):
        for v in range(DCTSIZE):
            if (u == 0):
                cu = 1 / math.sqrt(2)
            else:
                cu = 1
            if (v == 0):
                cv = 1 / math.sqrt(2)
            else:
                cv = 1
            suv = 0
            for y in range(DCTSIZE):
                for x in range(DCTSIZE):
                    suv = suv + (math.cos((2*x+1)*u*math.pi/16))*(math.cos((2*y+1)*v*math.pi/16))*block[8*x+y]
            suv = suv*1/4*cu*cv
            dctblock.append(int(suv))
    return dctblock

'''
方法名称： qt
功能描述： 量化
参数说明：
    block -- 输入8x8block dct结果
    qt    -- 量化表
返回值  ： 量化结果， 8x8 block
'''
def qt(dctblock, qt):
    qtblock = []
    for i in range(DCTBLOCKSIZE):
        q = round(dctblock[i] / float(qt[i]))
        qtblock.append(int(q))
    return qtblock

'''
方法名称： zigzag
功能描述： zigzag重排
参数说明：
    block -- 输入8x8block 量化结果
    zzt   -- zz表
返回值  ： 重排结果，EOB位置
'''
def zigzag(qtblock, zzt):
    zblock = []
    eob = 64
    for i in range(DCTBLOCKSIZE):
        zblock.append(0)
    for i in range(DCTBLOCKSIZE):
        zblock[zzt[i]] = qtblock[i]
    for i in range(DCTBLOCKSIZE):
        if (zblock[DCTBLOCKSIZE - i - 1] == 0):
            eob = DCTBLOCKSIZE - i -1
        else:
            break
    return zblock, eob


'''
方法名称： calcodelen
功能描述： 生成code len
参数说明：
    val, 范围 DCT_MIN_QUANTED ~ DCT_MAX_QUANTED
返回值  ： code len
'''
def calcodelen(val):
    val = abs(val)
    if (val == 0):
        code_len = 0
    elif (val == 1):
        code_len = 1
    elif (val >= 2) and (val <= 3):
        code_len = 2
    elif (val >= 4) and (val <= 7):
        code_len = 3
    elif (val >= 8) and (val <= 15):
        code_len = 4
    elif (val >= 16) and (val <= 31):
        code_len = 5
    elif (val >= 32) and (val <= 63):
        code_len = 6
    elif (val >= 64) and (val <= 127):
        code_len = 7
    elif (val >= 128) and (val <= 255):
        code_len = 8
    elif (val >= 256) and (val <= 511):
        code_len = 9
    elif (val >= 512) and (val <= 1023):
        code_len = 10
    elif (val >= 1024) and (val <= 2047):
        code_len = 11
    else:
        code_len = 12
    return code_len

'''
方法名称： buildvlit
功能描述： 生成vli table
参数说明：
返回值  : vlit
'''
def buildvlit():
    vlit = []
    for i in range(DC_MAX_QUANTED - DC_MIN_QUANTED + 1):
        val = i + DC_MIN_QUANTED
        code_len = calcodelen(val)
        vlit.append(code_len)
    return vlit

'''
方法名称： getvli
功能描述： 生成code_len and amplitude
参数说明：
返回值  :  amplitude, code_len
'''
def getvli(value, vlit):
    codelen = vlit[value - DC_MIN_QUANTED]
    if value >= 0:
        amplitude = value
    else:
        amplitude = abs(value) ^ 0xffff
    return amplitude, codelen

'''
方法名称： buildbuftab
功能描述： 根据标准hufman BITS and Value 生成bufman 表
参数说明：
    huf_bits  -- 
    huf_value -- 
返回值  ： huftab[][0] -- code; huftab[][1] -- length
'''
def buildhuftab(huf_bits, huf_value):
    huftab = []
    k = 0
    code = 0
    for i in range(256):
        huftab.append((0, 0))
    
    for i in range(16):
        for j in range(huf_bits[i]):
            huftab[huf_value[k]] = code, i + 1
            k = k + 1
            code = code + 1
        code = code * 2
    return huftab

'''
方法名称： rle
功能描述： 将交流信号进行RLE编码
参数说明：
    zblock -- 经过zigzag编码后的数据块
    eob -- block中最后非零数的位置
返回值  ： rleblock， 包含（前面0的数目， 数据）
'''
def rle(zblock, eob):
    zero_num = 0
    rleblock = []
    for i in range(eob - 1):
        if zblock[i+1] == 0:
            zero_num = zero_num + 1
            if zero_num == 16:
                zero_num = 0
                rleblock.append((16, 0))
        else:
            rleblock.append((zero_num, zblock[i+1]))
            zero_num = 0
    return rleblock        

'''
方法名称： jfifo
功能描述： 将变长编码的结果写入文件
参数说明：
    vlc_code -- 变长编码
    vlc_length -- 变长编码长度
    filename -- 写入文件名
返回值  ： 
'''
global bytenew
global bytepos
bytenew = 0
bytepos = 7

def jfifo(vlc_code, vlc_length, filename):
    global bytenew
    global bytepos
    #print "jfifo", vlc_code, vlc_length
    mask = [1,2,4,8,16,32,64,128,256,512,1024,2048,4096,8192,16384,32768] 
    for i in range(vlc_length):
        posval = vlc_length  - i - 1
        if vlc_code & mask[posval]:
            bytenew = bytenew | mask[bytepos]
        bytepos = bytepos - 1
        if bytepos < 0:
            if bytenew == 0xff:
                filename.write(chr(0xff))
                filename.write(chr(0x00))
            else:
                filename.write(chr(bytenew))
                #print "bytenew:%x; bytepos:%x"%(bytenew, bytepos)
            bytepos = 7
            bytenew = 0

'''
方法名称： jpg_wrsoi
功能描述： 写soi
参数说明：
    filename -- 写入文件名
返回值  ： 
'''
def jpg_wrsoi(filename):
    soi = []
    soi.append(0xff)
    soi.append(0xd8)
    for i in range(len(soi)):
        filename.write(chr(soi[i]))

'''
方法名称： jpg_wrapp0
功能描述： 写app0
参数说明：
    filename -- 写入文件名
返回值  ： 
'''
def jpg_wrapp0(filename):
    app0 = []
    app0.append(0xff)
    app0.append(0xe0) #0xFFE0 APP0
    app0.append(0x00)
    app0.append(0x10) #Application data segment length
    app0.append(ord('J'))
    app0.append(ord('F'))
    app0.append(ord('I'))
    app0.append(ord('F'))
    app0.append(0)
    app0.append(0x01)
    app0.append(0x01)
    app0.append(0x00)
    app0.append(0x00)
    app0.append(0x01)
    app0.append(0x00)
    app0.append(0x01)
    app0.append(0x00)
    app0.append(0x00)
    for i in range(len(app0)):
        filename.write(chr(app0[i]))

'''
方法名称： jpg_wrdqt
功能描述： 写dqt
参数说明：
    y_dqt --
    uv_dqt -- 
    filename -- 写入文件名
返回值  ： 
'''
def jpg_wrdqt(ydqt, uvdqt, filename):
    dqt = []
    dqt.append(0xff)
    dqt.append(0xdb) #0xFFDB define quantization table
    dqt.append(0x00) 
    dqt.append(0x43) #Quantization table definition length = 2 + (1+64)
    #append y qt
    dqt.append(0x00)
    for i in range(DCTBLOCKSIZE):
        dqt.append(ydqt[i])
    #append uv qt
    dqt.append(0xff)
    dqt.append(0xdb) #0xFFDB define quantization table
    dqt.append(0x00) 
    dqt.append(0x43) #Quantization table definition length = 2 + (1+64)
    dqt.append(0x01)
    for i in range(DCTBLOCKSIZE):
        dqt.append(uvdqt[i])
    #write file
    for i in range(len(dqt)):
        filename.write(chr(dqt[i]))
        
'''
方法名称： jpg_wrsof
功能描述： 写sof
参数说明：
    hsize -- 图像宽度
    vsize -- 图像高度
    filename -- 写入文件名
返回值  ： 
'''
def jpg_wrsof(hsize, vsize, filename):
    sof = []
    vsize_h = (vsize & 0xff00) >> 8
    vsize_l = (vsize & 0x00ff)
    hsize_h = (hsize & 0xff00) >> 8
    hsize_l = (hsize & 0x00ff)
    sof.append(0xff)
    sof.append(0xc0)
    sof.append(0x00)
    sof.append(0x11)
    sof.append(0x08)
    sof.append(vsize_h) #height
    sof.append(vsize_l)
    sof.append(hsize_h) #width
    sof.append(hsize_l)
    sof.append(0x03) #Y,U and V component
    sof.append(0x01) #Y ID
    sof.append(0x11) #Y sampling factor
    sof.append(0x00) #Y quantization table
    sof.append(0x02) #U ID
    sof.append(0x11) #U sampling factor
    sof.append(0x01) #Y quantization table 
    sof.append(0x03) #V ID 
    sof.append(0x11) #V sampling factor
    sof.append(0x01) #V quantization table
    for i in range(len(sof)):
        filename.write(chr(sof[i]))

'''
方法名称： jpg_wrdht
功能描述： 写dht
参数说明：
    y_dcht
    y_acht
    uv_dcht
    uv_acht
    filename -- 写入文件名
返回值  ： 
'''
def jpg_wrdht(filename):
    dht = []
    #append Y_DC
    dht.append(0xff)
    dht.append(0xc4)
    length = 19 + 12
    dht.append((length & 0xff00) >> 8)
    dht.append(length & 0xff)
    dht.append(0x00)
    for i in range(len(STD_Y_DC_HUFSIZE)):
        dht.append(STD_Y_DC_HUFSIZE[i])
    for i in range(len(STD_Y_DC_HUFVALU)):
        dht.append(STD_Y_DC_HUFVALU[i])
    #append UV_DC
    dht.append(0xff)
    dht.append(0xc4)
    length = 19 + 12
    dht.append((length & 0xff00) >> 8)
    dht.append(length & 0xff)
    dht.append(0x01)
    for i in range(len(STD_UV_DC_HUFSIZE)):
        dht.append(STD_UV_DC_HUFSIZE[i])
    for i in range(len(STD_UV_DC_HUFVALU)):
        dht.append(STD_UV_DC_HUFVALU[i])
    #append Y_AC
    dht.append(0xff)
    dht.append(0xc4)
    length = 19 + 162
    dht.append((length & 0xff00) >> 8)
    dht.append(length & 0xff)
    dht.append(0x10)
    for i in range(len(STD_Y_AC_HUFSIZE)):
        dht.append(STD_Y_AC_HUFSIZE[i])
    for i in range(len(STD_Y_AC_HUFVALU)):
        dht.append(STD_Y_AC_HUFVALU[i])
    #append UV_DC
    dht.append(0xff)
    dht.append(0xc4)
    length = 19 + 162
    dht.append((length & 0xff00) >> 8)
    dht.append(length & 0xff)
    dht.append(0x11)
    for i in range(len(STD_UV_AC_HUFSIZE)):
        dht.append(STD_UV_AC_HUFSIZE[i])
    for i in range(len(STD_UV_AC_HUFVALU)):
        dht.append(STD_UV_AC_HUFVALU[i])
    #write file
    for i in range(len(dht)):
        filename.write(chr(dht[i]))
    
'''
方法名称： jpg_wrsos
功能描述： 写sos
参数说明：
    filename -- 写入文件名
返回值  ： 
'''
def jpg_wrsos(filename):
    sos = []
    sos.append(0xff)
    sos.append(0xda) #SOS 
    sos.append(0x00)
    sos.append(0x0c) #length
    sos.append(0x03) #component num
    sos.append(0x01) #YID
    sos.append(0x00) #HTY
    sos.append(0x02) #UID
    sos.append(0x11) #HTU
    sos.append(0x03) #VID
    sos.append(0x11) #HTV
    sos.append(0x00) #SS
    sos.append(0x3f) #SE
    sos.append(0x00) #BF
    #write file
    for i in range(len(sos)):
        filename.write(chr(sos[i]))

'''
方法名称： jpg_wreoi
功能描述： 写eoi
参数说明：
    filename -- 写入文件名
返回值  ： 
'''
def jpg_wreoi(filename):
    eoi = []
    eoi.append(0xff)
    eoi.append(0xd9)
    for i in range(len(eoi)):
        filename.write(chr(eoi[i]))

'''
方法名称： jpg_process_data
功能描述： 完成图像编码
参数说明：
    y_buff
    u_buff
    v_buff
    filename -- 写入文件åﾐ
    y_dc_huf_table
    uv_dc_huf_table
    y_ac_huf_table
    uv_ac_huf_table
返回值  ： 
'''
def jpg_process_data(y_buff, u_buff, vbuff, filename, y_dc_huf_table, uv_dc_huf_table, y_ac_huf_table, uv_ac_huf_table, vlitable):
    y_buf = []
    u_buf = []
    v_buf = []

    global bytenew
    global bytepos
    bytenew = 0
    bytepos = 7
    
    for i in range(DCTBLOCKSIZE):
        y_buf.append(0)
    for i in range(DCTBLOCKSIZE):
        u_buf.append(0)
    for i in range(DCTBLOCKSIZE):
        v_buf.append(0)

    y_dc = 0
    u_dc = 0
    v_dc = 0
    mcu_num = len(v_buff) / DCTBLOCKSIZE
    for mcu_cnt in range(mcu_num):
        # do Y chn
        for i in range(DCTBLOCKSIZE):
            y_buf[i] = y_buff[mcu_cnt*DCTBLOCKSIZE + i] - 128
        dctblock = fdct(y_buf)
        qtblock = qt(dctblock, STD_Y_QT)
        zblock, eob = zigzag(qtblock, STD_ZIGZAGT)
        #直流编码
        diffval = int(zblock[0] - y_dc)
        y_dc = zblock[0]
        if diffval == 0:
            jfifo(y_dc_huf_table[0][0], y_dc_huf_table[0][1], filename)
        else:
            amplitude, code_len = getvli(diffval, vlitable)
            jfifo(y_dc_huf_table[code_len][0], y_dc_huf_table[code_len][1], filename)
            jfifo(amplitude, code_len, filename)
        #交流编码
        if eob == 1: #所有AC分量都为0
            jfifo(y_ac_huf_table[0][0], y_ac_huf_table[0][1], filename)
        else:
            rleblock = rle(zblock, eob)
            for i in range(len(rleblock)):
                if rleblock[i][0] == 16:
                    jfifo(y_ac_huf_table[0xf0][0], y_ac_huf_table[0xf0][1], filename)
                else:
                    amplitude, code_len = getvli(rleblock[i][1], vlitable)
                    jfifo(y_ac_huf_table[rleblock[i][0] * 16 + code_len][0], y_ac_huf_table[rleblock[i][0] * 16 + code_len][1], filename)
                    jfifo(amplitude, code_len, filename)
            if len(rleblock) != 63:
                jfifo(y_ac_huf_table[0][0], y_ac_huf_table[0][1], filename)
            	
        # do U chnfor i in range(DCTBLOCKSIZE):
        for i in range(DCTBLOCKSIZE):
            u_buf[i] = u_buff[mcu_cnt*DCTBLOCKSIZE + i] - 128
        dctblock = fdct(u_buf)
        qtblock = qt(dctblock, STD_UV_QT)
        zblock, eob = zigzag(qtblock, STD_ZIGZAGT)
        #直流编码
        diffval = zblock[0] - u_dc
        u_dc = zblock[0]
        if diffval == 0:
            jfifo(uv_dc_huf_table[0][0], uv_dc_huf_table[0][1], filename)
        else:
            amplitude, code_len = getvli(diffval, vlitable)
            jfifo(uv_dc_huf_table[code_len][0], uv_dc_huf_table[code_len][1], filename)
            jfifo(amplitude, code_len, filename)
        #交流编码
        if eob == 1: #所有AC分量都为0
            jfifo(uv_ac_huf_table[0][0], uv_ac_huf_table[0][1], filename)
        else:
            rleblock = rle(zblock, eob)
            for i in range(len(rleblock)):
                if rleblock[i][0] == 16:
                    jfifo(uv_ac_huf_table[0xf0][0], uv_ac_huf_table[0xf0][1], filename)
                else:
                    amplitude, code_len = getvli(rleblock[i][1], vlitable)
                    jfifo(uv_ac_huf_table[rleblock[i][0] * 16 + code_len][0], uv_ac_huf_table[rleblock[i][0] * 16 + code_len][1], filename)
                    jfifo(amplitude, code_len, filename)
            if len(rleblock) != 63:
                jfifo(uv_ac_huf_table[0][0], uv_ac_huf_table[0][1], filename)
       
        # do V chn
        for i in range(DCTBLOCKSIZE):
            v_buf[i] = v_buff[mcu_cnt*DCTBLOCKSIZE + i] - 128
        dctblock = fdct(v_buf)
        qtblock = qt(dctblock, STD_UV_QT)
        zblock, eob = zigzag(qtblock, STD_ZIGZAGT)
        #直流编码
        diffval = zblock[0] - v_dc
        v_dc = zblock[0]
        if diffval == 0:
            jfifo(uv_dc_huf_table[0][0], uv_dc_huf_table[0][1], filename)
        else:
            amplitude, code_len = getvli(diffval, vlitable)
            jfifo(uv_dc_huf_table[code_len][0], uv_dc_huf_table[code_len][1], filename)
            jfifo(amplitude, code_len, filename)
        #交流编码
        if eob == 1: #所有AC分量都为0
            jfifo(uv_ac_huf_table[0][0], uv_ac_huf_table[0][1], filename)
        else:
            rleblock = rle(zblock, eob)
            for i in range(len(rleblock)):
                if rleblock[i][0] == 16:
                    jfifo(uv_ac_huf_table[0xf0][0], uv_ac_huf_table[0xf0][1], filename)
                else:
                    amplitude, code_len = getvli(rleblock[i][1], vlitable)
                    jfifo(uv_ac_huf_table[rleblock[i][0] * 16 + code_len][0], uv_ac_huf_table[rleblock[i][0] * 16 + code_len][1], filename)
                    jfifo(amplitude, code_len, filename)
            if len(rleblock) != 63:
                jfifo(uv_ac_huf_table[0x00][0], uv_ac_huf_table[0x00][1], filename)

if __name__ == '__main__':
    img = img_op.open_img("../img/2.bmp")
    img_op.get_img_info(img)
    #提取图像信息
    img_hsize = img_op.get_img_info(img)[1]
    img_vsize = img_op.get_img_info(img)[0]
    #生成YUVbuffer
    yuv = img_op.rgb2yuv(img, 1)
    #拆分 Y,U,V buffer
    y_buff = []
    u_buff = []
    v_buff = []
    for i in range(len(yuv) / 3):
        y_buff.append(yuv[3*i + 0])
        u_buff.append(yuv[3*i + 1])
        v_buff.append(yuv[3*i + 2])
    #将buff组织成8x8block存储
    y_buff = div_buff(y_buff, img_hsize, img_vsize, DCTSIZE, DCTSIZE)
    u_buff = div_buff(u_buff, img_hsize, img_vsize, DCTSIZE, DCTSIZE)
    v_buff = div_buff(v_buff, img_hsize, img_vsize, DCTSIZE, DCTSIZE)
    #生成VLI表
    vlitable = buildvlit()
    #生成HUF表
    y_dc_huf_table  = buildhuftab(STD_Y_DC_HUFSIZE, STD_Y_DC_HUFVALU)
    uv_dc_huf_table = buildhuftab(STD_UV_DC_HUFSIZE, STD_UV_DC_HUFVALU)
    y_ac_huf_table  = buildhuftab(STD_Y_AC_HUFSIZE, STD_Y_AC_HUFVALU)
    uv_ac_huf_table = buildhuftab(STD_UV_AC_HUFSIZE, STD_UV_AC_HUFVALU)

    filename = "test.jpg"
    filename = open(filename, "wb")

    jpg_wrsoi(filename)
    jpg_wrapp0(filename)
    jpg_wrdqt(STD_Y_QT, STD_UV_QT, filename)
    jpg_wrsof(img_hsize, img_vsize, filename)
    jpg_wrdht(filename)
    jpg_wrsos(filename)
    # process image
    jpg_process_data(y_buff, u_buff, v_buff, filename, y_dc_huf_table, uv_dc_huf_table, y_ac_huf_table, uv_ac_huf_table, vlitable)
    jpg_wreoi(filename)
    filename.close()
