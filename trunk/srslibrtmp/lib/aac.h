#ifndef _AAC_H
#define _AAC_H
extern "C"{
#include <faad.h>
#include <faac.h>
}
#include <log.h>
#include <config.h>
class TAacCodec:public TConfig
{
  public:
    TAacCodec():
       //aac解码输出的采样率
      decodeOutputSampleRate(48000),
      decoderDataInit(false),
      //aac编码采样率
      encodeSampleRate(44100),
       //aac编码通道数
       encodeChannels(1),
      encodeInputSamples(0),
      encodeMaxOutputBytes(0)
    {}
    ~TAacCodec()
    {
      if(decoderHandle){
        NeAACDecClose(decoderHandle);
      }
      if(encoderHandle){
        faacEncClose(encoderHandle);
      }
    }
    void close()
    {
      if(decoderHandle){
        NeAACDecClose(decoderHandle);
      }
      if(encoderHandle){
        faacEncClose(encoderHandle);
      }
    }
    bool decodeInit()
    {
      //decode
      decoderHandle = NeAACDecOpen();
      if(!decoderHandle){
        return false;  
      }
      //参数配置
      NeAACDecConfigurationPtr conf = NeAACDecGetCurrentConfiguration(decoderHandle);
      if(!conf){
        return false;
      }
      conf->defSampleRate = decodeOutputSampleRate;
      conf->outputFormat = FAAD_FMT_16BIT;
      //很重要，禁止解码器更改采样率
      conf->dontUpSampleImplicitSBR = 1;
      NeAACDecSetConfiguration(decoderHandle, conf);
      decoderDataInit = false;
      return true;
    }
    bool encodeInit(int &aacInputSampleLength)
    {
      //encode
      encoderHandle = faacEncOpen(encodeSampleRate, encodeChannels, &encodeInputSamples, &encodeMaxOutputBytes);
      //81920太大了
      encodeMaxOutputBytes = 2048;
      aacInputSampleLength = encodeInputSamples*2;
      if(!encoderHandle)
      {
        LOG(ERROR, "encodeInit failed!!");
        return false;
      }
      faacEncConfigurationPtr conf = faacEncGetCurrentConfiguration(encoderHandle);
      conf->inputFormat = FAAC_INPUT_16BIT;
      faacEncSetConfiguration(encoderHandle, conf);
      return true;
    }
    //编码
    int encode(char* pcmBuffer,int pcmLength,unsigned char* aacBuffer)
    {
       int ret;
      //音频都是16bit
      int samples = pcmLength/(16/8);
      ret =  faacEncEncode(encoderHandle, (int*) pcmBuffer, samples, aacBuffer, encodeMaxOutputBytes);
      return ret;
    }
    //解码
    int decode(unsigned char *pData, int aacSize, unsigned char *pPCM, unsigned int *outLen)
    {
      int i,j;
      unsigned long smapleRate;
      unsigned char channels;
      if(!decoderDataInit){
        int res = NeAACDecInit(decoderHandle, pData, aacSize, &smapleRate, &channels);
        if(res<0){
          LOG(ERROR, "NeAACDecInit failed!!");
          return -1;
        }
        decoderDataInit = true;
      }
      
      unsigned char *buf = (unsigned char *)NeAACDecDecode(decoderHandle, &decodeInfo, pData, aacSize);
      if (buf && decodeInfo.error == 0){
        //faad解码出来都是双声道，将双声道改为单声道
        for(i=0,j=0; i<4096 && j<2048; i+=4, j+=2)
        {
            pPCM[j]= buf[i];
            pPCM[j+1]=buf[i+1];
        }
        *outLen = (unsigned int)decodeInfo.samples;
      }
      else{
        LOG(ERROR, "aacDecode failed,errno:%u--message:%s",decodeInfo.error,NeAACDecGetErrorMessage(decodeInfo.error));
        return -1;
      }
      return 0;
    }
  private:
    NeAACDecHandle decoderHandle;
    NeAACDecFrameInfo decodeInfo;
    int decodeOutputSampleRate;
    bool decoderDataInit;

    faacEncHandle encoderHandle;
    unsigned long encodeSampleRate;
    unsigned int encodeChannels;
    unsigned long encodeInputSamples;
    unsigned long encodeMaxOutputBytes;

};
#endif