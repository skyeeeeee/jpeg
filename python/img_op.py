# -*- coding: utf-8 -*-
"""
Created on Sat Jan 20 20:35:23 2018

@author: wang

@function: Basic image operations like open,write,show
"""

import cv2
import numpy as np
import os

def create_img(vsize, hsize, chn):
    img = np.zeros((vsize, hsize, chn), np.uint8)
    return img

def raw2rgb(img, raw_buffer):
    vsize, hsize, chn = img.shape
    for j in range(vsize):
        for i in range(hsize):
            if i%2 == 0 and j%2 == 0: 
                img[j][i][0] = raw_buffer[j*hsize + i] #append b
                img[j][i][1] = raw_buffer[j*hsize + i + 1] #append g 
                img[j][i][2] = raw_buffer[j*hsize + i + 1 + hsize] #append r
            elif i%2 == 1 and j%2 == 0: 
                img[j][i][0] = raw_buffer[j*hsize + i - 1] #append b
                img[j][i][1] = raw_buffer[j*hsize + i] #append g 
                img[j][i][2] = raw_buffer[j*hsize + i + hsize] #append r
            elif i%2 == 0 and j%2 == 1: 
                img[j][i][0] = raw_buffer[j*hsize + i - hsize] #append b
                img[j][i][1] = raw_buffer[j*hsize + i] #append g 
                img[j][i][2] = raw_buffer[j*hsize + i + 1] #append r
            elif i%2 == 1 and j%2 == 1: 
                img[j][i][0] = raw_buffer[j*hsize + i - 1 - hsize] #append b
                img[j][i][1] = raw_buffer[j*hsize + i - 1] #append g 
                img[j][i][2] = raw_buffer[j*hsize + i] #append r
    return img
    
'''
R = Y + 1.4075 * (V-128);  
G = Y - 0.3455 * (U-128) - 0.7169*(V-128);  
B = Y + 1.779 * (U-128);  
'''
def yuv2rgb(img, yuv_buffer, format_sel):
    vsize, hsize, chn = img.shape
    for j in range(vsize):
        for i in range(hsize):
            if(format_sel == 1): #yuv444
                y = yuv_buffer[j*hsize*3 + i*3 + 0]
                u = yuv_buffer[j*hsize*3 + i*3 + 1]
                v = yuv_buffer[j*hsize*3 + i*3 + 2]
            else: #yuv422
                if i%2 == 0:
                    y = yuv_buffer[j*hsize*2 + i*2 + 0]
                    u = yuv_buffer[j*hsize*2 + i*2 + 1]
                    v = yuv_buffer[j*hsize*2 + i*2 + 3]
                else:
                    y = yuv_buffer[j*hsize*2 + i*2 + 0]
                    u = yuv_buffer[j*hsize*2 + i*2 - 1]
                    v = yuv_buffer[j*hsize*2 + i*2 + 1]                    
            r = y + 1.4075*(v - 128)
            g = y - 0.3455*(u - 128) - 0.7169*(v-128)
            b = y + 1.799*(u - 128)
            img[j][i][0] = b
            img[j][i][1] = g
            img[j][i][2] = r
    return img

def open_img(file_name):
    print 'Supports bmp/jpg/png files input.'
    #file_name = raw_input()
    file_name_0 = os.path.basename(file_name)
    file_name_1, file_format = file_name_0.split('.')

    if (file_format == 'bmp') or (file_format == 'jpg'):
        img = cv2.imread(file_name, cv2.IMREAD_COLOR)        
    elif file_format == 'raw':
        raw_buffer = []
        file_in = open(file_name)
        for lines in file_in.readlines():
            lines = '0x'+lines.strip('\n')
            lines = int(lines,16)
            raw_buffer.append(lines)
        print "Enter image's shape(vsize,hsize,chn), like 1080,1920,3："
        vsize, hsize,chn = input()
        img = create_img(vsize, hsize, chn)
        img = raw2rgb(img, raw_buffer)
    elif file_format == 'yuv' or file_format == 'yuv422':
        yuv_buffer = []
        file_in = open(file_name)
        for lines in file_in.readlines():
            lines = '0x'+lines.strip('\n')
            lines = int(lines,16)
            yuv_buffer.append(lines)
        print "Enter image's shape(vsize,hsize,chn), like 1080,1920,3："
        vsize, hsize,chn = input()
        img = create_img(vsize, hsize, chn)
        img = yuv2rgb(img, yuv_buffer, 0)
    elif file_format == 'yuv444':
        yuv_buffer = []
        file_in = open(file_name)
        for lines in file_in.readlines():
            lines = '0x'+lines.strip('\n')
            lines = int(lines,16)
            yuv_buffer.append(lines)
        print "Enter image's shape(vsize,hsize,chn), like 1080,1920,3："
        vsize, hsize,chn = input()
        img = create_img(vsize, hsize, chn)
        img = yuv2rgb(img, yuv_buffer, 1)
    else:
        print "Don't support this format"
    cv2.imshow("img", img)
    cv2.waitKey(0)
    cv2.destroyAllWindows()
    return img
    
def get_img_info(img):
    print 'img shape info:', img.shape
    print 'img size info:', img.size
    print 'img data type info:', img.dtype
 
def rgb2raw(img):
    vsize, hsize, chn = img.shape
    raw_buffer = []
    for j in range(vsize):
        for i in range(hsize):
            if (i % 2 == 0) and (j % 2 == 0):
                raw_buffer.append(img[j][i][0]) # append b 
            elif (i % 2 == 1) and (j % 2 == 1):             
                raw_buffer.append(img[j][i][2]) # append r
            else:
                raw_buffer.append(img[j][i][1]) # append g 
    return raw_buffer

'''
Y =  0.299*R + 0.587*G + 0.114*B;
U = -0.169*R - 0.331*G + 0.5  *B + 128;     注： +128 的含义是让UV的范围处于整数区间（0-255）
V =  0.5  *R - 0.419*G - 0.081*B + 128;
YUV422 output seq: YUYV
YUV444 output seq: YUV
'''
def rgb2yuv(img, format_sel):
    vsize, hsize, chn = img.shape
    yuv_buffer = []
    for j in range(vsize):
        for i in range(hsize):
            b = img[j][i][0]
            g = img[j][i][1]
            r = img[j][i][2]
            y = 0.299*r + 0.587*g + 0.114*b
            u = -0.169*r - 0.331*g + 0.5*b + 128
            v = 0.5*r - 0.419*g -0.081*b + 128
            yuv_buffer.append(int(y)) #append y
            if format_sel == 0: #YUV422
                if (i%2 == 0):
                    yuv_buffer.append(int(u)) #append u
                else:
                    yuv_buffer.append(int(v)) #append v
            else: #YUV444
                yuv_buffer.append(int(u)) #append u
                yuv_buffer.append(int(v)) #append v
    return yuv_buffer
 
def write_img(img, file_name):
    print 'Supports bmp,jpg,yuv422,yuv444 and raw files output.'
    #file_name = raw_input()
    file_name = os.path.basename(file_name)
    file_name_0, file_format = file_name.split('.')
    if (file_format == 'bmp') or (file_format == 'jpg'):
        cv2.imwrite(file_name, img)
    elif file_format == 'raw':
        raw_buffer = rgb2raw(img)
        file_out = open(file_name, 'wb')
        for i in range(len(raw_buffer)):
            string = hex(raw_buffer[i])[2:]
            file_out.write(str(string)+'\n')
        file_out.close()
    elif (file_format == 'yuv') or (file_format == 'yuv422'):
        yuv_buffer = rgb2yuv(img, 0)
        file_out = open(file_name, 'wb')
        for i in range(len(yuv_buffer)):
            string = hex(yuv_buffer[i])[2:]
            file_out.write(str(string)+'\n')
        file_out.close()
    elif file_format == 'yuv444':
        yuv_buffer = rgb2yuv(img, 1)
        file_out = open(file_name, 'wb')
        for i in range(len(yuv_buffer)):
            string = hex(yuv_buffer[i])[2:]
            file_out.write(str(string)+'\n')
        file_out.close()
    else:
        print "don't support this format"

if __name__ == '__main__':
    img = open_img("../img/1.bmp")
    get_img_info(img)
    write_img(img, '1.raw')
    write_img(img, '1.yuv422')    
    write_img(img, '1.yuv444')