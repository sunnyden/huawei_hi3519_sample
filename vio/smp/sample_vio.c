#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include "sample_comm.h"

HI_VOID SAMPLE_VIO_MsgInit(HI_VOID)
{
}

HI_VOID SAMPLE_VIO_MsgExit(HI_VOID)
{
}

void SAMPLE_VIO_HandleSig(HI_S32 signo)
{
    signal(SIGINT, SIG_IGN);
    signal(SIGTERM, SIG_IGN);

    if (SIGINT == signo || SIGTERM == signo)
    {
        SAMPLE_COMM_VENC_StopGetStream();
        SAMPLE_COMM_All_ISP_Stop();
        SAMPLE_COMM_VO_HdmiStop();
        SAMPLE_COMM_SYS_Exit();
        printf("\033[0;31mprogram termination abnormally!\033[0;39m\n");
    }
    exit(-1);
}

HI_S32 SAMPLE_VIO_ViOnlineVpssOnlineRoute(HI_U32 u32VoIntfType)
{
    HI_S32             s32Ret;

    HI_S32             s32ViCnt       = 1;
    VI_DEV             ViDev          = 0;
    VI_PIPE            ViPipe         = 0;
    VI_CHN             ViChn          = 0;
    HI_S32             s32WorkSnsId   = 0;
    SAMPLE_VI_CONFIG_S stViConfig;

    SIZE_S             stSize;
    VB_CONFIG_S        stVbConf;
    PIC_SIZE_E         enPicSize;
    HI_U32             u32BlkSize;

    VO_CHN             VoChn          = 0;
    SAMPLE_VO_CONFIG_S stVoConfig;

    WDR_MODE_E         enWDRMode      = WDR_MODE_NONE;
    DYNAMIC_RANGE_E    enDynamicRange = DYNAMIC_RANGE_SDR8;
    PIXEL_FORMAT_E     enPixFormat    = PIXEL_FORMAT_YVU_SEMIPLANAR_420;
    VIDEO_FORMAT_E     enVideoFormat  = VIDEO_FORMAT_LINEAR;
    COMPRESS_MODE_E    enCompressMode = COMPRESS_MODE_NONE;

    VPSS_GRP           VpssGrp        = 0;
    VPSS_GRP_ATTR_S    stVpssGrpAttr;
    VPSS_CHN           VpssChn        = VPSS_CHN0;
    HI_BOOL            abChnEnable[VPSS_MAX_PHY_CHN_NUM] = {0};
    VPSS_CHN_ATTR_S    astVpssChnAttr[VPSS_MAX_PHY_CHN_NUM];

    VENC_CHN           VencChn[1]  = {0};
    PAYLOAD_TYPE_E     enType      = PT_H265;
    SAMPLE_RC_E        enRcMode    = SAMPLE_RC_CBR;
    HI_U32             u32Profile  = 0;
    VENC_GOP_ATTR_S    stGopAttr;

    /*config vi*/
    SAMPLE_COMM_VI_GetSensorInfo(&stViConfig);

    stViConfig.s32WorkingViNum                                   = s32ViCnt;
    stViConfig.as32WorkingViId[0]                                = 0;
    stViConfig.astViInfo[s32WorkSnsId].stSnsInfo.MipiDev         = ViDev;
    stViConfig.astViInfo[s32WorkSnsId].stDevInfo.ViDev           = ViDev;
    stViConfig.astViInfo[s32WorkSnsId].stDevInfo.enWDRMode       = enWDRMode;
    stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.enMastPipeMode = VI_ONLINE_VPSS_ONLINE;
    stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.aPipe[0]       = ViPipe;
    stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.aPipe[1]       = -1;
    stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.aPipe[2]       = -1;
    stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.aPipe[3]       = -1;
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.ViChn           = ViChn;
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.enPixFormat     = enPixFormat;
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.enDynamicRange  = enDynamicRange;
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.enVideoFormat   = enVideoFormat;
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.enCompressMode  = enCompressMode;

    /*get picture size*/
    s32Ret = SAMPLE_COMM_VI_GetSizeBySensor(stViConfig.astViInfo[s32WorkSnsId].stSnsInfo.enSnsType, &enPicSize);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("get picture size by sensor failed!\n");
        return s32Ret;
    }

    s32Ret = SAMPLE_COMM_SYS_GetPicSize(enPicSize, &stSize);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("get picture size failed!\n");
        return s32Ret;
    }

    /*config vb*/
    hi_memset(&stVbConf, sizeof(VB_CONFIG_S), 0, sizeof(VB_CONFIG_S));
    stVbConf.u32MaxPoolCnt              = 2;

    u32BlkSize = COMMON_GetPicBufferSize(stSize.u32Width, stSize.u32Height, SAMPLE_PIXEL_FORMAT, DATA_BITWIDTH_8, COMPRESS_MODE_SEG, DEFAULT_ALIGN);
    stVbConf.astCommPool[0].u64BlkSize  = u32BlkSize;
    stVbConf.astCommPool[0].u32BlkCnt   = 10;

    u32BlkSize = VI_GetRawBufferSize(stSize.u32Width, stSize.u32Height, PIXEL_FORMAT_RGB_BAYER_16BPP, COMPRESS_MODE_NONE, DEFAULT_ALIGN);
    stVbConf.astCommPool[1].u64BlkSize  = u32BlkSize;
    stVbConf.astCommPool[1].u32BlkCnt   = 4;

    s32Ret = SAMPLE_COMM_SYS_Init(&stVbConf);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("system init failed with %d!\n", s32Ret);
        return s32Ret;
    }

    /*start vi*/
    s32Ret = SAMPLE_COMM_VI_StartVi(&stViConfig);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("start vi failed.s32Ret:0x%x !\n", s32Ret);
        goto EXIT;
    }

    /*config vpss*/
    hi_memset(&stVpssGrpAttr, sizeof(VPSS_GRP_ATTR_S), 0, sizeof(VPSS_GRP_ATTR_S));
    stVpssGrpAttr.stFrameRate.s32SrcFrameRate    = -1;
    stVpssGrpAttr.stFrameRate.s32DstFrameRate    = -1;
    stVpssGrpAttr.enDynamicRange                 = DYNAMIC_RANGE_SDR8;
    stVpssGrpAttr.enPixelFormat                  = enPixFormat;
    stVpssGrpAttr.u32MaxW                        = stSize.u32Width;
    stVpssGrpAttr.u32MaxH                        = stSize.u32Height;
    stVpssGrpAttr.bNrEn                          = HI_TRUE;
    stVpssGrpAttr.stNrAttr.enCompressMode        = COMPRESS_MODE_FRAME;
    stVpssGrpAttr.stNrAttr.enNrMotionMode        = NR_MOTION_MODE_NORMAL;

    astVpssChnAttr[VpssChn].u32Width                    = stSize.u32Width;
    astVpssChnAttr[VpssChn].u32Height                   = stSize.u32Height;
    astVpssChnAttr[VpssChn].enChnMode                   = VPSS_CHN_MODE_USER;
    astVpssChnAttr[VpssChn].enCompressMode              = enCompressMode;
    astVpssChnAttr[VpssChn].enDynamicRange              = enDynamicRange;
    astVpssChnAttr[VpssChn].enVideoFormat               = enVideoFormat;
    astVpssChnAttr[VpssChn].enPixelFormat               = enPixFormat;
    astVpssChnAttr[VpssChn].stFrameRate.s32SrcFrameRate = 30;
    astVpssChnAttr[VpssChn].stFrameRate.s32DstFrameRate = 30;
    astVpssChnAttr[VpssChn].u32Depth                    = 0;
    astVpssChnAttr[VpssChn].bMirror                     = HI_FALSE;
    astVpssChnAttr[VpssChn].bFlip                       = HI_FALSE;
    astVpssChnAttr[VpssChn].stAspectRatio.enMode        = ASPECT_RATIO_NONE;

    /*start vpss*/
    abChnEnable[0] = HI_TRUE;
    s32Ret = SAMPLE_COMM_VPSS_Start(VpssGrp, abChnEnable, &stVpssGrpAttr, astVpssChnAttr);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("start vpss group failed. s32Ret: 0x%x !\n", s32Ret);
        goto EXIT1;
    }

    /*config venc */
    stGopAttr.enGopMode  = VENC_GOPMODE_SMARTP;
    stGopAttr.stSmartP.s32BgQpDelta  = 7;
    stGopAttr.stSmartP.s32ViQpDelta  = 2;
    stGopAttr.stSmartP.u32BgInterval = 1200;
    s32Ret = SAMPLE_COMM_VENC_Start(VencChn[0], enType, enPicSize, enRcMode, u32Profile, &stGopAttr);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("start venc failed. s32Ret: 0x%x !\n", s32Ret);
        goto EXIT2;
    }

    s32Ret = SAMPLE_COMM_VPSS_Bind_VENC(VpssGrp, VpssChn, VencChn[0]);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Venc bind Vpss failed. s32Ret: 0x%x !n", s32Ret);
        goto EXIT3;
    }

    /*config vo*/
    SAMPLE_COMM_VO_GetDefConfig(&stVoConfig);
    stVoConfig.enDstDynamicRange = enDynamicRange;
    if (1 == u32VoIntfType)
    {
        stVoConfig.enVoIntfType = VO_INTF_BT1120;
    }
    else
    {
        stVoConfig.enVoIntfType = VO_INTF_HDMI;
    }
    stVoConfig.enPicSize = enPicSize;

    /*start vo*/
    s32Ret = SAMPLE_COMM_VO_StartVO(&stVoConfig);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("start vo failed. s32Ret: 0x%x !\n", s32Ret);
        goto EXIT4;
    }

    /*vpss bind vo*/
    s32Ret = SAMPLE_COMM_VPSS_Bind_VO(VpssGrp, VpssChn, stVoConfig.VoDev, VoChn);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("vo bind vpss failed. s32Ret: 0x%x !\n", s32Ret);
        goto EXIT5;
    }

    s32Ret = SAMPLE_COMM_VENC_StartGetStream(VencChn, sizeof(VencChn)/sizeof(VENC_CHN));
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Get venc stream failed!\n");
        goto EXIT6;
    }

    PAUSE();

    SAMPLE_COMM_VENC_StopGetStream();

EXIT6:
    SAMPLE_COMM_VPSS_UnBind_VO(VpssGrp, VpssChn, stVoConfig.VoDev, VoChn);
EXIT5:
    SAMPLE_COMM_VO_StopVO(&stVoConfig);
EXIT4:
    SAMPLE_COMM_VPSS_UnBind_VENC(VpssGrp, VpssChn, VencChn[0]);
EXIT3:
    SAMPLE_COMM_VENC_Stop(VencChn[0]);
EXIT2:
    SAMPLE_COMM_VPSS_Stop(VpssGrp, abChnEnable);
EXIT1:
    SAMPLE_COMM_VI_StopVi(&stViConfig);
EXIT:
    SAMPLE_COMM_SYS_Exit();
    return s32Ret;
}

HI_S32 SAMPLE_VIO_ViOnlineVpssOfflineRoute(HI_U32 u32VoIntfType)
{
    HI_S32             s32Ret;

    HI_S32             s32ViCnt       = 1;
    VI_DEV             ViDev          = 0;
    VI_PIPE            ViPipe         = 0;
    VI_CHN             ViChn          = 0;
    HI_S32             s32WorkSnsId   = 0;
    SAMPLE_VI_CONFIG_S stViConfig;

    SIZE_S             stSize;
    VB_CONFIG_S        stVbConf;
    PIC_SIZE_E         enPicSize;
    HI_U32             u32BlkSize;

    VO_CHN             VoChn          = 0;
    SAMPLE_VO_CONFIG_S stVoConfig;

    WDR_MODE_E         enWDRMode      = WDR_MODE_NONE;
    DYNAMIC_RANGE_E    enDynamicRange = DYNAMIC_RANGE_SDR8;
    PIXEL_FORMAT_E     enPixFormat    = PIXEL_FORMAT_YVU_SEMIPLANAR_420;
    VIDEO_FORMAT_E     enVideoFormat  = VIDEO_FORMAT_LINEAR;
    COMPRESS_MODE_E    enCompressMode = COMPRESS_MODE_NONE;

    VPSS_GRP           VpssGrp        = 0;
    VPSS_GRP_ATTR_S    stVpssGrpAttr;
    VPSS_CHN           VpssChn        = VPSS_CHN0;
    HI_BOOL            abChnEnable[VPSS_MAX_PHY_CHN_NUM] = {0};
    VPSS_CHN_ATTR_S    astVpssChnAttr[VPSS_MAX_PHY_CHN_NUM];

    VENC_CHN           VencChn[1]  = {0};
    PAYLOAD_TYPE_E     enType      = PT_H265;
    SAMPLE_RC_E        enRcMode    = SAMPLE_RC_CBR;
    HI_U32             u32Profile  = 0;
    VENC_GOP_ATTR_S    stGopAttr;

    VI_LDC_ATTR_S      stLDCAttr = {0};
    DIS_CONFIG_S       stDISConfig = {0};
    DIS_ATTR_S         stDISAttr = {0};
    SPREAD_ATTR_S      stSpreadAttr = {0};

    /*config vi*/
    SAMPLE_COMM_VI_GetSensorInfo(&stViConfig);

    stViConfig.s32WorkingViNum                                   = s32ViCnt;
    stViConfig.as32WorkingViId[0]                                = 0;
    stViConfig.astViInfo[s32WorkSnsId].stSnsInfo.MipiDev         = ViDev;
    stViConfig.astViInfo[s32WorkSnsId].stDevInfo.ViDev           = ViDev;
    stViConfig.astViInfo[s32WorkSnsId].stDevInfo.enWDRMode       = enWDRMode;
    stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.enMastPipeMode = VI_OFFLINE_VPSS_OFFLINE;
    stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.aPipe[0]       = ViPipe;
    stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.aPipe[1]       = -1;
    stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.aPipe[2]       = -1;
    stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.aPipe[3]       = -1;
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.ViChn           = ViChn;
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.enPixFormat     = enPixFormat;
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.enDynamicRange  = enDynamicRange;
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.enVideoFormat   = enVideoFormat;
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.enCompressMode  = enCompressMode;

    /*get picture size*/
    s32Ret = SAMPLE_COMM_VI_GetSizeBySensor(stViConfig.astViInfo[s32WorkSnsId].stSnsInfo.enSnsType, &enPicSize);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("get picture size by sensor failed!\n");
        return s32Ret;
    }

    s32Ret = SAMPLE_COMM_SYS_GetPicSize(enPicSize, &stSize);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("get picture size failed!\n");
        return s32Ret;
    }

    /*config vb*/
    hi_memset(&stVbConf, sizeof(VB_CONFIG_S), 0, sizeof(VB_CONFIG_S));
    stVbConf.u32MaxPoolCnt              = 2;

    u32BlkSize = COMMON_GetPicBufferSize(stSize.u32Width, stSize.u32Height, SAMPLE_PIXEL_FORMAT, DATA_BITWIDTH_8, COMPRESS_MODE_SEG, DEFAULT_ALIGN);
    stVbConf.astCommPool[0].u64BlkSize  = u32BlkSize;
    stVbConf.astCommPool[0].u32BlkCnt   = 10;

    u32BlkSize = VI_GetRawBufferSize(stSize.u32Width, stSize.u32Height, PIXEL_FORMAT_RGB_BAYER_16BPP, COMPRESS_MODE_NONE, DEFAULT_ALIGN);
    stVbConf.astCommPool[1].u64BlkSize  = u32BlkSize;
    stVbConf.astCommPool[1].u32BlkCnt   = 4;

    s32Ret = SAMPLE_COMM_SYS_Init(&stVbConf);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("system init failed with %d!\n", s32Ret);
        return s32Ret;
    }

    /*start vi*/
    s32Ret = SAMPLE_COMM_VI_StartVi(&stViConfig);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("start vi failed.s32Ret:0x%x !\n", s32Ret);
        goto EXIT;
    }

    /*config vpss*/
    hi_memset(&stVpssGrpAttr, sizeof(VPSS_GRP_ATTR_S), 0, sizeof(VPSS_GRP_ATTR_S));
    stVpssGrpAttr.stFrameRate.s32SrcFrameRate    = -1;
    stVpssGrpAttr.stFrameRate.s32DstFrameRate    = -1;
    stVpssGrpAttr.enDynamicRange                 = DYNAMIC_RANGE_SDR8;
    stVpssGrpAttr.enPixelFormat                  = enPixFormat;
    stVpssGrpAttr.u32MaxW                        = stSize.u32Width;
    stVpssGrpAttr.u32MaxH                        = stSize.u32Height;
    stVpssGrpAttr.bNrEn                          = HI_TRUE;
    stVpssGrpAttr.stNrAttr.enCompressMode        = COMPRESS_MODE_FRAME;
    stVpssGrpAttr.stNrAttr.enNrMotionMode        = NR_MOTION_MODE_NORMAL;

    astVpssChnAttr[VpssChn].u32Width                    = stSize.u32Width;
    astVpssChnAttr[VpssChn].u32Height                   = stSize.u32Height;
    astVpssChnAttr[VpssChn].enChnMode                   = VPSS_CHN_MODE_USER;
    astVpssChnAttr[VpssChn].enCompressMode              = enCompressMode;
    astVpssChnAttr[VpssChn].enDynamicRange              = enDynamicRange;
    astVpssChnAttr[VpssChn].enVideoFormat               = enVideoFormat;
    astVpssChnAttr[VpssChn].enPixelFormat               = enPixFormat;
    astVpssChnAttr[VpssChn].stFrameRate.s32SrcFrameRate = 30;
    astVpssChnAttr[VpssChn].stFrameRate.s32DstFrameRate = 30;
    astVpssChnAttr[VpssChn].u32Depth                    = 0;
    astVpssChnAttr[VpssChn].bMirror                     = HI_FALSE;
    astVpssChnAttr[VpssChn].bFlip                       = HI_FALSE;
    astVpssChnAttr[VpssChn].stAspectRatio.enMode        = ASPECT_RATIO_NONE;

    /*start vpss*/
    abChnEnable[0] = HI_TRUE;
    s32Ret = SAMPLE_COMM_VPSS_Start(VpssGrp, abChnEnable, &stVpssGrpAttr, astVpssChnAttr);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("start vpss group failed. s32Ret: 0x%x !\n", s32Ret);
        goto EXIT1;
    }

    /*vi bind vpss*/
    s32Ret = SAMPLE_COMM_VI_Bind_VPSS(ViPipe, ViChn, VpssGrp);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("vi bind vpss failed. s32Ret: 0x%x !\n", s32Ret);
        goto EXIT2;
    }

    /*config venc */
    stGopAttr.enGopMode  = VENC_GOPMODE_SMARTP;
    stGopAttr.stSmartP.s32BgQpDelta  = 7;
    stGopAttr.stSmartP.s32ViQpDelta  = 2;
    stGopAttr.stSmartP.u32BgInterval = 1200;
    s32Ret = SAMPLE_COMM_VENC_Start(VencChn[0], enType, enPicSize, enRcMode, u32Profile, &stGopAttr);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("start venc failed. s32Ret: 0x%x !\n", s32Ret);
        goto EXIT3;
    }

    s32Ret = SAMPLE_COMM_VPSS_Bind_VENC(VpssGrp, VpssChn, VencChn[0]);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Venc bind Vpss failed. s32Ret: 0x%x !n", s32Ret);
        goto EXIT4;
    }

    /*config vo*/
    SAMPLE_COMM_VO_GetDefConfig(&stVoConfig);
    stVoConfig.enDstDynamicRange = enDynamicRange;
    if (1 == u32VoIntfType)
    {
        stVoConfig.enVoIntfType = VO_INTF_BT1120;
    }
    else
    {
        stVoConfig.enVoIntfType = VO_INTF_HDMI;
    }
    stVoConfig.enPicSize = enPicSize;

    /*start vo*/
    s32Ret = SAMPLE_COMM_VO_StartVO(&stVoConfig);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("start vo failed. s32Ret: 0x%x !\n", s32Ret);
        goto EXIT5;
    }

    /*vpss bind vo*/
    s32Ret = SAMPLE_COMM_VPSS_Bind_VO(VpssGrp, VpssChn, stVoConfig.VoDev, VoChn);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("vo bind vpss failed. s32Ret: 0x%x !\n", s32Ret);
        goto EXIT6;
    }

    s32Ret = SAMPLE_COMM_VENC_StartGetStream(VencChn, sizeof(VencChn)/sizeof(VENC_CHN));
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Get venc stream failed!\n");
        goto EXIT7;
    }

    printf("Press Enter key to Enable LDC!\n");
    getchar();

    stLDCAttr.bEnable = HI_TRUE;
    stLDCAttr.stAttr.bAspect = 0;
    stLDCAttr.stAttr.s32XRatio = 100;
    stLDCAttr.stAttr.s32YRatio = 100;
    stLDCAttr.stAttr.s32XYRatio = 100;
    stLDCAttr.stAttr.s32CenterXOffset = 0;
    stLDCAttr.stAttr.s32CenterYOffset = 0;
    stLDCAttr.stAttr.s32DistortionRatio = 500;

    s32Ret = HI_MPI_VI_SetChnLDCAttr(ViPipe, ViChn, &stLDCAttr);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("HI_MPI_VI_SetChnLDCAttr failed witfh %d\n", s32Ret);
        goto EXIT8;
    }

    printf("Press Enter key to Enable DIS!\n");
    getchar();

    stDISConfig.enMode              = DIS_MODE_6_DOF_GME;
    stDISConfig.enMotionLevel       = DIS_MOTION_LEVEL_NORMAL;
    stDISConfig.u32CropRatio        = 80;
    stDISConfig.u32BufNum           = 5;
    stDISConfig.enPdtType           = DIS_PDT_TYPE_IPC;
    stDISConfig.u32GyroOutputRange  = 0;
    stDISConfig.u32FrameRate        = 30;
    stDISConfig.bScale              = HI_TRUE;
    stDISConfig.bCameraSteady       = HI_FALSE;

    s32Ret = HI_MPI_VI_SetChnDISConfig(ViPipe, ViChn, &stDISConfig);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("HI_MPI_VI_SetChnDISConfig failed.s32Ret:0x%x !\n", s32Ret);
        goto EXIT5;
    }

    stDISAttr.bEnable               = HI_TRUE;
    stDISAttr.u32MovingSubjectLevel = 0;
    stDISAttr.s32RollingShutterCoef = 0;
    stDISAttr.u32ViewAngle          = 1000;
    stDISAttr.bStillCrop            = HI_FALSE;
    stDISAttr.u32HorizontalLimit    = 512;
    stDISAttr.u32VerticalLimit      = 512;

    s32Ret = HI_MPI_VI_SetChnDISAttr(ViPipe, ViChn, &stDISAttr);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("HI_MPI_VI_SetChnDISAttr failed.s32Ret:0x%x !\n", s32Ret);
        goto EXIT5;
    }

    printf("Press Enter key to Enable Spread!\n");
    getchar();

    stSpreadAttr.bEnable        = HI_TRUE;
    stSpreadAttr.u32SpreadCoef  = 16;
    stSpreadAttr.stDestSize.u32Width = 3840;
    stSpreadAttr.stDestSize.u32Height = 2160;

    s32Ret = HI_MPI_VI_SetChnSpreadAttr(ViPipe, ViChn, &stSpreadAttr);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("HI_MPI_VI_SetChnSpreadAttr failed witfh %d\n", s32Ret);
        goto EXIT5;
    }

    PAUSE();

EXIT8:
    SAMPLE_COMM_VENC_StopGetStream();
EXIT7:
    SAMPLE_COMM_VPSS_UnBind_VO(VpssGrp, VpssChn, stVoConfig.VoDev, VoChn);
EXIT6:
    SAMPLE_COMM_VO_StopVO(&stVoConfig);
EXIT5:
    SAMPLE_COMM_VPSS_UnBind_VENC(VpssGrp, VpssChn, VencChn[0]);
EXIT4:
    SAMPLE_COMM_VENC_Stop(VencChn[0]);
EXIT3:
    SAMPLE_COMM_VI_UnBind_VPSS(ViPipe, ViChn, VpssGrp);
EXIT2:
    SAMPLE_COMM_VPSS_Stop(VpssGrp, abChnEnable);
EXIT1:
    SAMPLE_COMM_VI_StopVi(&stViConfig);
EXIT:
    SAMPLE_COMM_SYS_Exit();
    return s32Ret;
}

HI_S32 SAMPLE_VIO_ViDoublePipeRoute(HI_U32 u32VoIntfType)
{
    HI_S32             s32Ret;

    HI_S32             s32ViCnt       = 1;
    VI_DEV             ViDev          = 0;
    VI_PIPE            ViPipe[2]      = {0, 1};
    VI_CHN             ViChn          = 0;
    HI_S32             s32WorkSnsId   = 0;
    SAMPLE_VI_CONFIG_S stViConfig;

    SIZE_S             stSize;
    VB_CONFIG_S        stVbConf;
    PIC_SIZE_E         enPicSize;
    HI_U32             u32BlkSize;

    VO_CHN             VoChn          = 0;
    SAMPLE_VO_CONFIG_S stVoConfig;

    WDR_MODE_E         enWDRMode      = WDR_MODE_NONE;
    DYNAMIC_RANGE_E    enDynamicRange = DYNAMIC_RANGE_SDR8;
    PIXEL_FORMAT_E     enPixFormat    = PIXEL_FORMAT_YVU_SEMIPLANAR_420;
    VIDEO_FORMAT_E     enVideoFormat  = VIDEO_FORMAT_LINEAR;
    COMPRESS_MODE_E    enCompressMode = COMPRESS_MODE_NONE;

    VPSS_GRP           VpssGrp[2]     = {0, 1};
    VPSS_GRP_ATTR_S    stVpssGrpAttr;
    VPSS_CHN           VpssChn        = VPSS_CHN0;
    HI_BOOL            abChnEnable[VPSS_MAX_PHY_CHN_NUM] = {0};
    VPSS_CHN_ATTR_S    astVpssChnAttr[VPSS_MAX_PHY_CHN_NUM];

    VENC_CHN           VencChn[1]  = {0};
    PAYLOAD_TYPE_E     enType      = PT_H265;
    SAMPLE_RC_E        enRcMode    = SAMPLE_RC_CBR;
    HI_U32             u32Profile  = 0;
    VENC_GOP_ATTR_S    stGopAttr;

    /*config vi*/
    SAMPLE_COMM_VI_GetSensorInfo(&stViConfig);

    stViConfig.s32WorkingViNum                                   = s32ViCnt;
    stViConfig.as32WorkingViId[0]                                = 0;
    stViConfig.astViInfo[s32WorkSnsId].stSnsInfo.MipiDev         = ViDev;
    stViConfig.astViInfo[s32WorkSnsId].stDevInfo.ViDev           = ViDev;
    stViConfig.astViInfo[s32WorkSnsId].stDevInfo.enWDRMode       = enWDRMode;
    stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.enMastPipeMode = VI_OFFLINE_VPSS_ONLINE;
    stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.aPipe[0]       = ViPipe[0];
    stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.aPipe[1]       = ViPipe[1];
    stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.aPipe[2]       = -1;
    stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.aPipe[3]       = -1;
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.ViChn           = ViChn;
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.enPixFormat     = enPixFormat;
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.enDynamicRange  = enDynamicRange;
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.enVideoFormat   = enVideoFormat;
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.enCompressMode  = enCompressMode;

    stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.bMultiPipe     = HI_TRUE;

    /*get picture size*/
    s32Ret = SAMPLE_COMM_VI_GetSizeBySensor(stViConfig.astViInfo[s32WorkSnsId].stSnsInfo.enSnsType, &enPicSize);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("get picture size by sensor failed!\n");
        return s32Ret;
    }

    s32Ret = SAMPLE_COMM_SYS_GetPicSize(enPicSize, &stSize);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("get picture size failed!\n");
        return s32Ret;
    }

    /*config vb*/
    hi_memset(&stVbConf, sizeof(VB_CONFIG_S), 0, sizeof(VB_CONFIG_S));
    stVbConf.u32MaxPoolCnt              = 2;

    u32BlkSize = COMMON_GetPicBufferSize(stSize.u32Width, stSize.u32Height, SAMPLE_PIXEL_FORMAT, DATA_BITWIDTH_8, COMPRESS_MODE_SEG, DEFAULT_ALIGN);
    stVbConf.astCommPool[0].u64BlkSize  = u32BlkSize;
    stVbConf.astCommPool[0].u32BlkCnt   = 20;

    u32BlkSize = VI_GetRawBufferSize(stSize.u32Width, stSize.u32Height, PIXEL_FORMAT_RGB_BAYER_16BPP, COMPRESS_MODE_NONE, DEFAULT_ALIGN);
    stVbConf.astCommPool[1].u64BlkSize  = u32BlkSize;
    stVbConf.astCommPool[1].u32BlkCnt   = 4;

    s32Ret = SAMPLE_COMM_SYS_Init(&stVbConf);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("system init failed with %d!\n", s32Ret);
        return s32Ret;
    }

    /*start vi*/
    s32Ret = SAMPLE_COMM_VI_StartVi(&stViConfig);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("start vi failed.s32Ret:0x%x !\n", s32Ret);
        goto EXIT;
    }

    /*config vpss*/
    hi_memset(&stVpssGrpAttr, sizeof(VPSS_GRP_ATTR_S), 0, sizeof(VPSS_GRP_ATTR_S));
    stVpssGrpAttr.stFrameRate.s32SrcFrameRate    = -1;
    stVpssGrpAttr.stFrameRate.s32DstFrameRate    = -1;
    stVpssGrpAttr.enDynamicRange                 = DYNAMIC_RANGE_SDR8;
    stVpssGrpAttr.enPixelFormat                  = enPixFormat;
    stVpssGrpAttr.u32MaxW                        = stSize.u32Width;
    stVpssGrpAttr.u32MaxH                        = stSize.u32Height;
    stVpssGrpAttr.bNrEn                          = HI_TRUE;
    stVpssGrpAttr.stNrAttr.enCompressMode        = COMPRESS_MODE_FRAME;
    stVpssGrpAttr.stNrAttr.enNrMotionMode        = NR_MOTION_MODE_NORMAL;

    astVpssChnAttr[VpssChn].u32Width                    = stSize.u32Width;
    astVpssChnAttr[VpssChn].u32Height                   = stSize.u32Height;
    astVpssChnAttr[VpssChn].enChnMode                   = VPSS_CHN_MODE_USER;
    astVpssChnAttr[VpssChn].enCompressMode              = enCompressMode;
    astVpssChnAttr[VpssChn].enDynamicRange              = enDynamicRange;
    astVpssChnAttr[VpssChn].enVideoFormat               = enVideoFormat;
    astVpssChnAttr[VpssChn].enPixelFormat               = enPixFormat;
    astVpssChnAttr[VpssChn].stFrameRate.s32SrcFrameRate = 30;
    astVpssChnAttr[VpssChn].stFrameRate.s32DstFrameRate = 30;
    astVpssChnAttr[VpssChn].u32Depth                    = 0;
    astVpssChnAttr[VpssChn].bMirror                     = HI_FALSE;
    astVpssChnAttr[VpssChn].bFlip                       = HI_FALSE;
    astVpssChnAttr[VpssChn].stAspectRatio.enMode        = ASPECT_RATIO_NONE;

    /*start vpss*/
    abChnEnable[0] = HI_TRUE;
    s32Ret = SAMPLE_COMM_VPSS_Start(VpssGrp[0], abChnEnable, &stVpssGrpAttr, astVpssChnAttr);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("start vpss group failed. s32Ret: 0x%x !\n", s32Ret);
        goto EXIT1;
    }

    s32Ret = SAMPLE_COMM_VPSS_Start(VpssGrp[1], abChnEnable, &stVpssGrpAttr, astVpssChnAttr);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("start vpss group failed. s32Ret: 0x%x !\n", s32Ret);
        goto EXIT2;
    }

    /*config venc */
    stGopAttr.enGopMode  = VENC_GOPMODE_SMARTP;
    stGopAttr.stSmartP.s32BgQpDelta  = 7;
    stGopAttr.stSmartP.s32ViQpDelta  = 2;
    stGopAttr.stSmartP.u32BgInterval = 1200;
    s32Ret = SAMPLE_COMM_VENC_Start(VencChn[0], enType, enPicSize, enRcMode, u32Profile, &stGopAttr);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("start venc failed. s32Ret: 0x%x !\n", s32Ret);
        goto EXIT3;
    }

    s32Ret = SAMPLE_COMM_VPSS_Bind_VENC(VpssGrp[1], VpssChn, VencChn[0]);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Venc bind Vpss failed. s32Ret: 0x%x !n", s32Ret);
        goto EXIT4;
    }

    /*config vo*/
    SAMPLE_COMM_VO_GetDefConfig(&stVoConfig);
    stVoConfig.enDstDynamicRange = enDynamicRange;
    if (1 == u32VoIntfType)
    {
        stVoConfig.enVoIntfType = VO_INTF_BT1120;
    }
    else
    {
        stVoConfig.enVoIntfType = VO_INTF_HDMI;
    }
    stVoConfig.enPicSize = enPicSize;

    /*start vo*/
    s32Ret = SAMPLE_COMM_VO_StartVO(&stVoConfig);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("start vo failed. s32Ret: 0x%x !\n", s32Ret);
        goto EXIT5;
    }

    /*vpss bind vo*/
    s32Ret = SAMPLE_COMM_VPSS_Bind_VO(VpssGrp[0], VpssChn, stVoConfig.VoDev, VoChn);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("vo bind vpss failed. s32Ret: 0x%x !\n", s32Ret);
        goto EXIT6;
    }

    s32Ret = SAMPLE_COMM_VENC_StartGetStream(VencChn, sizeof(VencChn)/sizeof(VENC_CHN));
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Get venc stream failed!\n");
        goto EXIT7;
    }

    PAUSE();

    SAMPLE_COMM_VENC_StopGetStream();

EXIT7:
    SAMPLE_COMM_VPSS_UnBind_VO(VpssGrp[0], VpssChn, stVoConfig.VoDev, VoChn);
EXIT6:
    SAMPLE_COMM_VO_StopVO(&stVoConfig);
EXIT5:
    SAMPLE_COMM_VPSS_UnBind_VENC(VpssGrp[1], VpssChn, VencChn[0]);
EXIT4:
    SAMPLE_COMM_VENC_Stop(VencChn[0]);
EXIT3:
    SAMPLE_COMM_VPSS_Stop(VpssGrp[1], abChnEnable);
EXIT2:
    SAMPLE_COMM_VPSS_Stop(VpssGrp[0], abChnEnable);
EXIT1:
    SAMPLE_COMM_VI_StopVi(&stViConfig);
EXIT:
    SAMPLE_COMM_SYS_Exit();
    return s32Ret;
}

HI_S32 SAMPLE_VIO_ViDoubleChnRoute(HI_U32 u32VoIntfType)
{
    HI_S32             s32Ret;

    HI_S32             s32ViCnt       = 1;
    VI_DEV             ViDev          = 0;
    VI_PIPE            ViPipe         = 0;
    VI_CHN             ViChn[2]       = {0, 1};
    HI_S32             s32WorkSnsId   = 0;
    SAMPLE_VI_CONFIG_S stViConfig;
    VI_CHN_ATTR_S      stChnAttr;
    VI_EARLY_INTERRUPT_S stEarlyInterrupt;

    SIZE_S             stSize;
    VB_CONFIG_S        stVbConf;
    PIC_SIZE_E         enPicSize;
    HI_U32             u32BlkSize;

    VO_CHN             VoChn          = 0;
    SAMPLE_VO_CONFIG_S stVoConfig;

    WDR_MODE_E         enWDRMode      = WDR_MODE_NONE;
    DYNAMIC_RANGE_E    enDynamicRange = DYNAMIC_RANGE_SDR8;
    PIXEL_FORMAT_E     enPixFormat    = PIXEL_FORMAT_YVU_SEMIPLANAR_420;
    VIDEO_FORMAT_E     enVideoFormat  = VIDEO_FORMAT_LINEAR;
    COMPRESS_MODE_E    enCompressMode = COMPRESS_MODE_NONE;

    VPSS_GRP           VpssGrp        = 0;
    VPSS_GRP_ATTR_S    stVpssGrpAttr;
    VPSS_CHN           VpssChn        = VPSS_CHN0;
    HI_BOOL            abChnEnable[VPSS_MAX_PHY_CHN_NUM] = {0};
    VPSS_CHN_ATTR_S    astVpssChnAttr[VPSS_MAX_PHY_CHN_NUM];

    VENC_CHN           VencChn[1]  = {0};
    PAYLOAD_TYPE_E     enType      = PT_H265;
    SAMPLE_RC_E        enRcMode    = SAMPLE_RC_CBR;
    HI_U32             u32Profile  = 0;
    VENC_GOP_ATTR_S    stGopAttr;

    /*config vi*/
    SAMPLE_COMM_VI_GetSensorInfo(&stViConfig);

    stViConfig.s32WorkingViNum                                   = s32ViCnt;
    stViConfig.as32WorkingViId[0]                                = 0;
    stViConfig.astViInfo[s32WorkSnsId].stSnsInfo.MipiDev         = ViDev;
    stViConfig.astViInfo[s32WorkSnsId].stDevInfo.ViDev           = ViDev;
    stViConfig.astViInfo[s32WorkSnsId].stDevInfo.enWDRMode       = enWDRMode;
    stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.enMastPipeMode = VI_ONLINE_VPSS_OFFLINE;
    stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.aPipe[0]       = ViPipe;
    stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.aPipe[1]       = -1;
    stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.aPipe[2]       = -1;
    stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.aPipe[3]       = -1;
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.ViChn           = ViChn[0];
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.enPixFormat     = enPixFormat;
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.enDynamicRange  = enDynamicRange;
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.enVideoFormat   = enVideoFormat;
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.enCompressMode  = enCompressMode;

    /*get picture size*/
    s32Ret = SAMPLE_COMM_VI_GetSizeBySensor(stViConfig.astViInfo[s32WorkSnsId].stSnsInfo.enSnsType, &enPicSize);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("get picture size by sensor failed!\n");
        return s32Ret;
    }

    s32Ret = SAMPLE_COMM_SYS_GetPicSize(enPicSize, &stSize);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("get picture size failed!\n");
        return s32Ret;
    }

    /*config vb*/
    hi_memset(&stVbConf, sizeof(VB_CONFIG_S), 0, sizeof(VB_CONFIG_S));
    stVbConf.u32MaxPoolCnt              = 2;

    u32BlkSize = COMMON_GetPicBufferSize(stSize.u32Width, stSize.u32Height, SAMPLE_PIXEL_FORMAT, DATA_BITWIDTH_8, COMPRESS_MODE_SEG, DEFAULT_ALIGN);
    stVbConf.astCommPool[0].u64BlkSize  = u32BlkSize;
    stVbConf.astCommPool[0].u32BlkCnt   = 10;

    u32BlkSize = VI_GetRawBufferSize(stSize.u32Width, stSize.u32Height, PIXEL_FORMAT_RGB_BAYER_16BPP, COMPRESS_MODE_NONE, DEFAULT_ALIGN);
    stVbConf.astCommPool[1].u64BlkSize  = u32BlkSize;
    stVbConf.astCommPool[1].u32BlkCnt   = 4;

    s32Ret = SAMPLE_COMM_SYS_Init(&stVbConf);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("system init failed with %d!\n", s32Ret);
        return s32Ret;
    }

    /*start vi*/
    s32Ret = SAMPLE_COMM_VI_StartVi(&stViConfig);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("start vi failed.s32Ret:0x%x !\n", s32Ret);
        goto EXIT;
    }

    /*config vpss*/
    hi_memset(&stVpssGrpAttr, sizeof(VPSS_GRP_ATTR_S), 0, sizeof(VPSS_GRP_ATTR_S));
    stVpssGrpAttr.stFrameRate.s32SrcFrameRate    = -1;
    stVpssGrpAttr.stFrameRate.s32DstFrameRate    = -1;
    stVpssGrpAttr.enDynamicRange                 = DYNAMIC_RANGE_SDR8;
    stVpssGrpAttr.enPixelFormat                  = enPixFormat;
    stVpssGrpAttr.u32MaxW                        = stSize.u32Width;
    stVpssGrpAttr.u32MaxH                        = stSize.u32Height;
    stVpssGrpAttr.bNrEn                          = HI_TRUE;
    stVpssGrpAttr.stNrAttr.enCompressMode        = COMPRESS_MODE_FRAME;
    stVpssGrpAttr.stNrAttr.enNrMotionMode        = NR_MOTION_MODE_NORMAL;

    astVpssChnAttr[VpssChn].u32Width                    = stSize.u32Width;
    astVpssChnAttr[VpssChn].u32Height                   = stSize.u32Height;
    astVpssChnAttr[VpssChn].enChnMode                   = VPSS_CHN_MODE_USER;
    astVpssChnAttr[VpssChn].enCompressMode              = enCompressMode;
    astVpssChnAttr[VpssChn].enDynamicRange              = enDynamicRange;
    astVpssChnAttr[VpssChn].enVideoFormat               = enVideoFormat;
    astVpssChnAttr[VpssChn].enPixelFormat               = enPixFormat;
    astVpssChnAttr[VpssChn].stFrameRate.s32SrcFrameRate = 30;
    astVpssChnAttr[VpssChn].stFrameRate.s32DstFrameRate = 30;
    astVpssChnAttr[VpssChn].u32Depth                    = 0;
    astVpssChnAttr[VpssChn].bMirror                     = HI_FALSE;
    astVpssChnAttr[VpssChn].bFlip                       = HI_FALSE;
    astVpssChnAttr[VpssChn].stAspectRatio.enMode        = ASPECT_RATIO_NONE;

    /*start vpss*/
    abChnEnable[0] = HI_TRUE;
    s32Ret = SAMPLE_COMM_VPSS_Start(VpssGrp, abChnEnable, &stVpssGrpAttr, astVpssChnAttr);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("start vpss group failed. s32Ret: 0x%x !\n", s32Ret);
        goto EXIT1;
    }

    /*vi bind vpss*/
    s32Ret = SAMPLE_COMM_VI_Bind_VPSS(ViPipe, ViChn[0], VpssGrp);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("vi bind vpss failed. s32Ret: 0x%x !\n", s32Ret);
        goto EXIT2;
    }

    /*start vi chn1*/
    s32Ret = HI_MPI_VI_GetChnAttr(ViPipe, ViChn[0], &stChnAttr);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("HI_MPI_VI_GetChnAttr failed. s32Ret: 0x%x !\n", s32Ret);
        goto EXIT3;
    }

    stChnAttr.stSize.u32Width = stSize.u32Width / 2;
    stChnAttr.stSize.u32Height = stSize.u32Height / 2;

    s32Ret = HI_MPI_VI_SetChnAttr(ViPipe, ViChn[1], &stChnAttr);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("HI_MPI_VI_GetChnAttr failed. s32Ret: 0x%x !\n", s32Ret);
        goto EXIT3;
    }

    stEarlyInterrupt.bEnable = HI_TRUE;
    stEarlyInterrupt.u32LineCnt = stChnAttr.stSize.u32Height / 2;
    s32Ret = HI_MPI_VI_SetChnEarlyInterrupt(ViPipe, ViChn[1], &stEarlyInterrupt);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("HI_MPI_VI_SetChnEarlyInterrupt failed. s32Ret: 0x%x !\n", s32Ret);
        goto EXIT3;
    }

    s32Ret = HI_MPI_VI_EnableChn(ViPipe, ViChn[1]);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("HI_MPI_VI_EnableChn failed. s32Ret: 0x%x !\n", s32Ret);
        goto EXIT3;
    }

    /*config venc */
    stGopAttr.enGopMode  = VENC_GOPMODE_SMARTP;
    stGopAttr.stSmartP.s32BgQpDelta  = 7;
    stGopAttr.stSmartP.s32ViQpDelta  = 2;
    stGopAttr.stSmartP.u32BgInterval = 1200;
    s32Ret = SAMPLE_COMM_VENC_Start(VencChn[0], enType, enPicSize, enRcMode, u32Profile, &stGopAttr);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("start venc failed. s32Ret: 0x%x !\n", s32Ret);
        goto EXIT4;
    }

    s32Ret = SAMPLE_COMM_VPSS_Bind_VENC(VpssGrp, VpssChn, VencChn[0]);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Venc bind Vpss failed. s32Ret: 0x%x !n", s32Ret);
        goto EXIT5;
    }

    /*config vo*/
    SAMPLE_COMM_VO_GetDefConfig(&stVoConfig);
    stVoConfig.enDstDynamicRange = enDynamicRange;
    if (1 == u32VoIntfType)
    {
        stVoConfig.enVoIntfType = VO_INTF_BT1120;
    }
    else
    {
        stVoConfig.enVoIntfType = VO_INTF_HDMI;
    }
    stVoConfig.enPicSize = enPicSize;

    /*start vo*/
    s32Ret = SAMPLE_COMM_VO_StartVO(&stVoConfig);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("start vo failed. s32Ret: 0x%x !\n", s32Ret);
        goto EXIT6;
    }

    /*vpss bind vo*/
    s32Ret = SAMPLE_COMM_VI_Bind_VO(ViPipe, ViChn[1], stVoConfig.VoDev, VoChn);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("vo bind vpss failed. s32Ret: 0x%x !\n", s32Ret);
        goto EXIT7;
    }

    s32Ret = SAMPLE_COMM_VENC_StartGetStream(VencChn, sizeof(VencChn)/sizeof(VENC_CHN));
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Get venc stream failed!\n");
        goto EXIT8;
    }

    PAUSE();

    SAMPLE_COMM_VENC_StopGetStream();

EXIT8:
    SAMPLE_COMM_VI_UnBind_VO(ViPipe, ViChn[1], stVoConfig.VoDev, VoChn);
EXIT7:
    SAMPLE_COMM_VO_StopVO(&stVoConfig);
EXIT6:
    HI_MPI_VI_DisableChn(ViPipe, ViChn[1]);
EXIT5:
    SAMPLE_COMM_VPSS_UnBind_VENC(VpssGrp, VpssChn, VencChn[0]);
EXIT4:
    SAMPLE_COMM_VENC_Stop(VencChn[0]);
EXIT3:
    SAMPLE_COMM_VI_UnBind_VPSS(ViPipe, ViChn[0], VpssGrp);
EXIT2:
    SAMPLE_COMM_VPSS_Stop(VpssGrp, abChnEnable);
EXIT1:
    SAMPLE_COMM_VI_StopVi(&stViConfig);
EXIT:
    SAMPLE_COMM_SYS_Exit();
    return s32Ret;
}

HI_S32 SAMPLE_VIO_ViWdrSwitch(HI_U32 u32VoIntfType)
{
    HI_S32             s32Ret;

    HI_S32             s32ViCnt       = 1;
    VI_DEV             ViDev          = 0;
    VI_PIPE            ViPipe         = 0;
    VI_CHN             ViChn          = 0;
    HI_S32             s32WorkSnsId   = 0;
    SAMPLE_VI_CONFIG_S stViConfig;

    SIZE_S             stSize;
    VB_CONFIG_S        stVbConf;
    PIC_SIZE_E         enPicSize;
    HI_U32             u32BlkSize;

    VO_CHN             VoChn          = 0;
    SAMPLE_VO_CONFIG_S stVoConfig;

    WDR_MODE_E         enWDRMode      = WDR_MODE_NONE;
    DYNAMIC_RANGE_E    enDynamicRange = DYNAMIC_RANGE_SDR8;
    PIXEL_FORMAT_E     enPixFormat    = PIXEL_FORMAT_YVU_SEMIPLANAR_420;
    VIDEO_FORMAT_E     enVideoFormat  = VIDEO_FORMAT_LINEAR;
    COMPRESS_MODE_E    enCompressMode = COMPRESS_MODE_NONE;

    VPSS_GRP           VpssGrp        = 0;
    VPSS_GRP_ATTR_S    stVpssGrpAttr;
    VPSS_CHN           VpssChn        = VPSS_CHN0;
    HI_BOOL            abChnEnable[VPSS_MAX_PHY_CHN_NUM] = {0};
    VPSS_CHN_ATTR_S    astVpssChnAttr[VPSS_MAX_PHY_CHN_NUM];

    VENC_CHN           VencChn[1]  = {0};
    PAYLOAD_TYPE_E     enType      = PT_H265;
    SAMPLE_RC_E        enRcMode    = SAMPLE_RC_CBR;
    HI_U32             u32Profile  = 0;
    VENC_GOP_ATTR_S    stGopAttr;

    if (SONY_IMX377_MIPI_8M_30FPS_10BIT == SENSOR0_TYPE)
    {
        SAMPLE_PRT("Sensor Not Support!\n");
        return HI_SUCCESS;
    }

    /*config vi*/
    SAMPLE_COMM_VI_GetSensorInfo(&stViConfig);

    stViConfig.s32WorkingViNum                                   = s32ViCnt;
    stViConfig.as32WorkingViId[0]                                = 0;
    stViConfig.astViInfo[s32WorkSnsId].stSnsInfo.MipiDev         = ViDev;
    stViConfig.astViInfo[s32WorkSnsId].stDevInfo.ViDev           = ViDev;
    stViConfig.astViInfo[s32WorkSnsId].stDevInfo.enWDRMode       = enWDRMode;
    stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.enMastPipeMode = VI_ONLINE_VPSS_OFFLINE;
    stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.aPipe[0]       = ViPipe;
    stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.aPipe[1]       = -1;
    stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.aPipe[2]       = -1;
    stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.aPipe[3]       = -1;
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.ViChn           = ViChn;
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.enPixFormat     = enPixFormat;
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.enDynamicRange  = enDynamicRange;
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.enVideoFormat   = enVideoFormat;
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.enCompressMode  = enCompressMode;

    /*get picture size*/
    s32Ret = SAMPLE_COMM_VI_GetSizeBySensor(stViConfig.astViInfo[s32WorkSnsId].stSnsInfo.enSnsType, &enPicSize);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("get picture size by sensor failed!\n");
        return s32Ret;
    }

    s32Ret = SAMPLE_COMM_SYS_GetPicSize(enPicSize, &stSize);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("get picture size failed!\n");
        return s32Ret;
    }

    /*config vb*/
    hi_memset(&stVbConf, sizeof(VB_CONFIG_S), 0, sizeof(VB_CONFIG_S));
    stVbConf.u32MaxPoolCnt              = 2;

    u32BlkSize = COMMON_GetPicBufferSize(stSize.u32Width, stSize.u32Height, SAMPLE_PIXEL_FORMAT, DATA_BITWIDTH_8, COMPRESS_MODE_SEG, DEFAULT_ALIGN);
    stVbConf.astCommPool[0].u64BlkSize  = u32BlkSize;
    stVbConf.astCommPool[0].u32BlkCnt   = 10;

    u32BlkSize = VI_GetRawBufferSize(stSize.u32Width, stSize.u32Height, PIXEL_FORMAT_RGB_BAYER_16BPP, COMPRESS_MODE_NONE, DEFAULT_ALIGN);
    stVbConf.astCommPool[1].u64BlkSize  = u32BlkSize;
    stVbConf.astCommPool[1].u32BlkCnt   = 4;

    s32Ret = SAMPLE_COMM_SYS_Init(&stVbConf);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("system init failed with %d!\n", s32Ret);
        return s32Ret;
    }

    /*start vi*/
    s32Ret = SAMPLE_COMM_VI_StartVi(&stViConfig);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("start vi failed.s32Ret:0x%x !\n", s32Ret);
        goto EXIT;
    }

    /*config vpss*/
    hi_memset(&stVpssGrpAttr, sizeof(VPSS_GRP_ATTR_S), 0, sizeof(VPSS_GRP_ATTR_S));
    stVpssGrpAttr.stFrameRate.s32SrcFrameRate    = -1;
    stVpssGrpAttr.stFrameRate.s32DstFrameRate    = -1;
    stVpssGrpAttr.enDynamicRange                 = DYNAMIC_RANGE_SDR8;
    stVpssGrpAttr.enPixelFormat                  = enPixFormat;
    stVpssGrpAttr.u32MaxW                        = stSize.u32Width;
    stVpssGrpAttr.u32MaxH                        = stSize.u32Height;
    stVpssGrpAttr.bNrEn                          = HI_FALSE;
    stVpssGrpAttr.stNrAttr.enCompressMode        = COMPRESS_MODE_FRAME;
    stVpssGrpAttr.stNrAttr.enNrMotionMode        = NR_MOTION_MODE_NORMAL;

    astVpssChnAttr[VpssChn].u32Width                    = stSize.u32Width;
    astVpssChnAttr[VpssChn].u32Height                   = stSize.u32Height;
    astVpssChnAttr[VpssChn].enChnMode                   = VPSS_CHN_MODE_USER;
    astVpssChnAttr[VpssChn].enCompressMode              = enCompressMode;
    astVpssChnAttr[VpssChn].enDynamicRange              = enDynamicRange;
    astVpssChnAttr[VpssChn].enVideoFormat               = enVideoFormat;
    astVpssChnAttr[VpssChn].enPixelFormat               = enPixFormat;
    astVpssChnAttr[VpssChn].stFrameRate.s32SrcFrameRate = 30;
    astVpssChnAttr[VpssChn].stFrameRate.s32DstFrameRate = 30;
    astVpssChnAttr[VpssChn].u32Depth                    = 0;
    astVpssChnAttr[VpssChn].bMirror                     = HI_FALSE;
    astVpssChnAttr[VpssChn].bFlip                       = HI_FALSE;
    astVpssChnAttr[VpssChn].stAspectRatio.enMode        = ASPECT_RATIO_NONE;

    /*start vpss*/
    abChnEnable[0] = HI_TRUE;
    s32Ret = SAMPLE_COMM_VPSS_Start(VpssGrp, abChnEnable, &stVpssGrpAttr, astVpssChnAttr);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("start vpss group failed. s32Ret: 0x%x !\n", s32Ret);
        goto EXIT1;
    }

    /*vi bind vpss*/
    s32Ret = SAMPLE_COMM_VI_Bind_VPSS(ViPipe, ViChn, VpssGrp);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("vi bind vpss failed. s32Ret: 0x%x !\n", s32Ret);
        goto EXIT2;
    }

    /*config venc */
    stGopAttr.enGopMode  = VENC_GOPMODE_SMARTP;
    stGopAttr.stSmartP.s32BgQpDelta  = 7;
    stGopAttr.stSmartP.s32ViQpDelta  = 2;
    stGopAttr.stSmartP.u32BgInterval = 1200;
    s32Ret = SAMPLE_COMM_VENC_Start(VencChn[0], enType, enPicSize, enRcMode, u32Profile, &stGopAttr);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("start venc failed. s32Ret: 0x%x !\n", s32Ret);
        goto EXIT3;
    }

    s32Ret = SAMPLE_COMM_VPSS_Bind_VENC(VpssGrp, VpssChn, VencChn[0]);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Venc bind Vpss failed. s32Ret: 0x%x !n", s32Ret);
        goto EXIT4;
    }

    /*config vo*/
    SAMPLE_COMM_VO_GetDefConfig(&stVoConfig);
    stVoConfig.enDstDynamicRange = enDynamicRange;
    if (1 == u32VoIntfType)
    {
        stVoConfig.enVoIntfType = VO_INTF_BT1120;
    }
    else
    {
        stVoConfig.enVoIntfType = VO_INTF_HDMI;
    }
    stVoConfig.enPicSize = enPicSize;

    /*start vo*/
    s32Ret = SAMPLE_COMM_VO_StartVO(&stVoConfig);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("start vo failed. s32Ret: 0x%x !\n", s32Ret);
        goto EXIT5;
    }

    /*vpss bind vo*/
    s32Ret = SAMPLE_COMM_VPSS_Bind_VO(VpssGrp, VpssChn, stVoConfig.VoDev, VoChn);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("vo bind vpss failed. s32Ret: 0x%x !\n", s32Ret);
        goto EXIT6;
    }

    s32Ret = SAMPLE_COMM_VENC_StartGetStream(VencChn, sizeof(VencChn)/sizeof(VENC_CHN));
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Get venc stream failed!\n");
        goto EXIT7;
    }

    printf("switch to wdr mode========\n");
    getchar();

    SAMPLE_COMM_VPSS_Stop(VpssGrp, abChnEnable);
    SAMPLE_COMM_VI_SwitchMode_StopVI(&stViConfig);

    if (SONY_IMX290_MIPI_2M_30FPS_12BIT == stViConfig.astViInfo[s32WorkSnsId].stSnsInfo.enSnsType)
    {
        stViConfig.astViInfo[s32WorkSnsId].stSnsInfo.enSnsType = SONY_IMX290_MIPI_2M_30FPS_12BIT_WDR2TO1;
    }
    else if (SONY_IMX334_MIPI_8M_30FPS_12BIT == stViConfig.astViInfo[s32WorkSnsId].stSnsInfo.enSnsType)
    {
        stViConfig.astViInfo[s32WorkSnsId].stSnsInfo.enSnsType = SONY_IMX334_MIPI_8M_30FPS_12BIT_WDR2TO1;
    }

    stViConfig.astViInfo[s32WorkSnsId].stDevInfo.enWDRMode = WDR_MODE_2To1_LINE;
    stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.aPipe[0]       = ViPipe;
    stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.aPipe[1]       = 1;

    SAMPLE_COMM_VI_SwitchMode(&stViConfig);

    s32Ret = SAMPLE_COMM_VPSS_Start(VpssGrp, abChnEnable, &stVpssGrpAttr, astVpssChnAttr);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("start vpss group failed. s32Ret: 0x%x !\n", s32Ret);
        goto EXIT8;
    }

    printf("switch to linear mode========\n");
    getchar();

    SAMPLE_COMM_VPSS_Stop(VpssGrp, abChnEnable);
    SAMPLE_COMM_VI_SwitchMode_StopVI(&stViConfig);

    stViConfig.astViInfo[s32WorkSnsId].stSnsInfo.enSnsType = SENSOR0_TYPE;
    stViConfig.astViInfo[s32WorkSnsId].stDevInfo.enWDRMode = WDR_MODE_NONE;
    stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.aPipe[0]       = ViPipe;
    stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.aPipe[1]       = -1;

    SAMPLE_COMM_VI_SwitchMode(&stViConfig);
    s32Ret = SAMPLE_COMM_VPSS_Start(VpssGrp, abChnEnable, &stVpssGrpAttr, astVpssChnAttr);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("start vpss group failed. s32Ret: 0x%x !\n", s32Ret);
        goto EXIT8;
    }

    PAUSE();

EXIT8:
    SAMPLE_COMM_VENC_StopGetStream();
EXIT7:
    SAMPLE_COMM_VPSS_UnBind_VO(VpssGrp, VpssChn, stVoConfig.VoDev, VoChn);
EXIT6:
    SAMPLE_COMM_VO_StopVO(&stVoConfig);
EXIT5:
    SAMPLE_COMM_VPSS_UnBind_VENC(VpssGrp, VpssChn, VencChn[0]);
EXIT4:
    SAMPLE_COMM_VENC_Stop(VencChn[0]);
EXIT3:
    SAMPLE_COMM_VI_UnBind_VPSS(ViPipe, ViChn, VpssGrp);
EXIT2:
    SAMPLE_COMM_VPSS_Stop(VpssGrp, abChnEnable);
EXIT1:
    SAMPLE_COMM_VI_StopVi(&stViConfig);
EXIT:
    SAMPLE_COMM_SYS_Exit();
    return s32Ret;
}


HI_S32 SAMPLE_VIO_ViVpssLowDelay(HI_U32 u32VoIntfType)
{
    HI_S32             s32Ret;

    HI_S32             s32ViCnt       = 1;
    VI_DEV             ViDev          = 0;
    VI_PIPE            ViPipe         = 0;
    VI_CHN             ViChn          = 0;
    HI_S32             s32WorkSnsId   = 0;
    SAMPLE_VI_CONFIG_S stViConfig;

    SIZE_S             stSize;
    VB_CONFIG_S        stVbConf;
    PIC_SIZE_E         enPicSize;
    HI_U32             u32BlkSize;

    VO_CHN             VoChn          = 0;
    SAMPLE_VO_CONFIG_S stVoConfig;

    WDR_MODE_E         enWDRMode      = WDR_MODE_NONE;
    DYNAMIC_RANGE_E    enDynamicRange = DYNAMIC_RANGE_SDR8;
    PIXEL_FORMAT_E     enPixFormat    = PIXEL_FORMAT_YVU_SEMIPLANAR_420;
    VIDEO_FORMAT_E     enVideoFormat  = VIDEO_FORMAT_LINEAR;
    COMPRESS_MODE_E    enCompressMode = COMPRESS_MODE_NONE;

    VPSS_GRP           VpssGrp        = 0;
    VPSS_GRP_ATTR_S    stVpssGrpAttr;
    VPSS_CHN           VpssChn        = VPSS_CHN0;
    HI_BOOL            abChnEnable[VPSS_MAX_PHY_CHN_NUM] = {0};
    VPSS_CHN_ATTR_S    astVpssChnAttr[VPSS_MAX_PHY_CHN_NUM];

    VENC_CHN           VencChn[1]  = {0};
    PAYLOAD_TYPE_E     enType      = PT_H265;
    SAMPLE_RC_E        enRcMode    = SAMPLE_RC_CBR;
    HI_U32             u32Profile  = 0;
    VENC_GOP_ATTR_S    stGopAttr;

    VI_LDC_ATTR_S      stLDCAttr = {0};
    VI_LOW_DELAY_INFO_S stLowDelayInfo;

    /*config vi*/
    SAMPLE_COMM_VI_GetSensorInfo(&stViConfig);

    stViConfig.s32WorkingViNum                                   = s32ViCnt;
    stViConfig.as32WorkingViId[0]                                = 0;
    stViConfig.astViInfo[s32WorkSnsId].stSnsInfo.MipiDev         = ViDev;
    stViConfig.astViInfo[s32WorkSnsId].stDevInfo.ViDev           = ViDev;
    stViConfig.astViInfo[s32WorkSnsId].stDevInfo.enWDRMode       = enWDRMode;
    stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.enMastPipeMode = VI_OFFLINE_VPSS_OFFLINE;
    stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.aPipe[0]       = ViPipe;
    stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.aPipe[1]       = -1;
    stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.aPipe[2]       = -1;
    stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.aPipe[3]       = -1;
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.ViChn           = ViChn;
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.enPixFormat     = enPixFormat;
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.enDynamicRange  = enDynamicRange;
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.enVideoFormat   = enVideoFormat;
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.enCompressMode  = enCompressMode;

    /*get picture size*/
    s32Ret = SAMPLE_COMM_VI_GetSizeBySensor(stViConfig.astViInfo[s32WorkSnsId].stSnsInfo.enSnsType, &enPicSize);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("get picture size by sensor failed!\n");
        return s32Ret;
    }

    s32Ret = SAMPLE_COMM_SYS_GetPicSize(enPicSize, &stSize);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("get picture size failed!\n");
        return s32Ret;
    }

    /*config vb*/
    hi_memset(&stVbConf, sizeof(VB_CONFIG_S), 0, sizeof(VB_CONFIG_S));
    stVbConf.u32MaxPoolCnt              = 2;

    u32BlkSize = COMMON_GetPicBufferSize(stSize.u32Width, stSize.u32Height, SAMPLE_PIXEL_FORMAT, DATA_BITWIDTH_8, COMPRESS_MODE_SEG, DEFAULT_ALIGN);
    stVbConf.astCommPool[0].u64BlkSize  = u32BlkSize;
    stVbConf.astCommPool[0].u32BlkCnt   = 10;

    u32BlkSize = VI_GetRawBufferSize(stSize.u32Width, stSize.u32Height, PIXEL_FORMAT_RGB_BAYER_16BPP, COMPRESS_MODE_NONE, DEFAULT_ALIGN);
    stVbConf.astCommPool[1].u64BlkSize  = u32BlkSize;
    stVbConf.astCommPool[1].u32BlkCnt   = 4;

    s32Ret = SAMPLE_COMM_SYS_Init(&stVbConf);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("system init failed with %d!\n", s32Ret);
        return s32Ret;
    }

    /*start vi*/
    s32Ret = SAMPLE_COMM_VI_StartVi(&stViConfig);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("start vi failed.s32Ret:0x%x !\n", s32Ret);
        goto EXIT;
    }

    /*config vpss*/
    hi_memset(&stVpssGrpAttr, sizeof(VPSS_GRP_ATTR_S), 0, sizeof(VPSS_GRP_ATTR_S));
    stVpssGrpAttr.stFrameRate.s32SrcFrameRate    = -1;
    stVpssGrpAttr.stFrameRate.s32DstFrameRate    = -1;
    stVpssGrpAttr.enDynamicRange                 = DYNAMIC_RANGE_SDR8;
    stVpssGrpAttr.enPixelFormat                  = enPixFormat;
    stVpssGrpAttr.u32MaxW                        = stSize.u32Width;
    stVpssGrpAttr.u32MaxH                        = stSize.u32Height;
    stVpssGrpAttr.bNrEn                          = HI_TRUE;
    stVpssGrpAttr.stNrAttr.enCompressMode        = COMPRESS_MODE_FRAME;
    stVpssGrpAttr.stNrAttr.enNrMotionMode        = NR_MOTION_MODE_NORMAL;

    astVpssChnAttr[VpssChn].u32Width                    = stSize.u32Width;
    astVpssChnAttr[VpssChn].u32Height                   = stSize.u32Height;
    astVpssChnAttr[VpssChn].enChnMode                   = VPSS_CHN_MODE_USER;
    astVpssChnAttr[VpssChn].enCompressMode              = enCompressMode;
    astVpssChnAttr[VpssChn].enDynamicRange              = enDynamicRange;
    astVpssChnAttr[VpssChn].enVideoFormat               = enVideoFormat;
    astVpssChnAttr[VpssChn].enPixelFormat               = enPixFormat;
    astVpssChnAttr[VpssChn].stFrameRate.s32SrcFrameRate = 30;
    astVpssChnAttr[VpssChn].stFrameRate.s32DstFrameRate = 30;
    astVpssChnAttr[VpssChn].u32Depth                    = 0;
    astVpssChnAttr[VpssChn].bMirror                     = HI_FALSE;
    astVpssChnAttr[VpssChn].bFlip                       = HI_FALSE;
    astVpssChnAttr[VpssChn].stAspectRatio.enMode        = ASPECT_RATIO_NONE;

    /*start vpss*/
    abChnEnable[0] = HI_TRUE;
    s32Ret = SAMPLE_COMM_VPSS_Start(VpssGrp, abChnEnable, &stVpssGrpAttr, astVpssChnAttr);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("start vpss group failed. s32Ret: 0x%x !\n", s32Ret);
        goto EXIT1;
    }

    /*vi bind vpss*/
    s32Ret = SAMPLE_COMM_VI_Bind_VPSS(ViPipe, ViChn, VpssGrp);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("vi bind vpss failed. s32Ret: 0x%x !\n", s32Ret);
        goto EXIT2;
    }

    /*config venc */
    stGopAttr.enGopMode  = VENC_GOPMODE_SMARTP;
    stGopAttr.stSmartP.s32BgQpDelta  = 7;
    stGopAttr.stSmartP.s32ViQpDelta  = 2;
    stGopAttr.stSmartP.u32BgInterval = 1200;
    s32Ret = SAMPLE_COMM_VENC_Start(VencChn[0], enType, enPicSize, enRcMode, u32Profile, &stGopAttr);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("start venc failed. s32Ret: 0x%x !\n", s32Ret);
        goto EXIT3;
    }

    s32Ret = SAMPLE_COMM_VPSS_Bind_VENC(VpssGrp, VpssChn, VencChn[0]);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Venc bind Vpss failed. s32Ret: 0x%x !n", s32Ret);
        goto EXIT4;
    }

    /*config vo*/
    SAMPLE_COMM_VO_GetDefConfig(&stVoConfig);
    stVoConfig.enDstDynamicRange = enDynamicRange;
    if (1 == u32VoIntfType)
    {
        stVoConfig.enVoIntfType = VO_INTF_BT1120;
    }
    else
    {
        stVoConfig.enVoIntfType = VO_INTF_HDMI;
    }
    stVoConfig.enPicSize = enPicSize;

    /*start vo*/
    s32Ret = SAMPLE_COMM_VO_StartVO(&stVoConfig);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("start vo failed. s32Ret: 0x%x !\n", s32Ret);
        goto EXIT5;
    }

    /*vpss bind vo*/
    s32Ret = SAMPLE_COMM_VPSS_Bind_VO(VpssGrp, VpssChn, stVoConfig.VoDev, VoChn);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("vo bind vpss failed. s32Ret: 0x%x !\n", s32Ret);
        goto EXIT6;
    }

    s32Ret = SAMPLE_COMM_VENC_StartGetStream(VencChn, sizeof(VencChn)/sizeof(VENC_CHN));
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Get venc stream failed!\n");
        goto EXIT7;
    }


    stLDCAttr.bEnable = HI_TRUE;
    stLDCAttr.stAttr.bAspect = 0;
    stLDCAttr.stAttr.s32XRatio = 100;
    stLDCAttr.stAttr.s32YRatio = 100;
    stLDCAttr.stAttr.s32XYRatio = 100;
    stLDCAttr.stAttr.s32CenterXOffset = 0;
    stLDCAttr.stAttr.s32CenterYOffset = 0;
    stLDCAttr.stAttr.s32DistortionRatio = 500;
    s32Ret = HI_MPI_VI_SetChnLDCAttr(ViPipe, ViChn, &stLDCAttr);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("HI_MPI_VI_SetChnLDCAttr failed witfh %d\n", s32Ret);
        goto EXIT8;
    }

    printf("Press Enter key to Enable LowDelay!\n");
    getchar();

    stLowDelayInfo.bEnable = HI_TRUE;
    stLowDelayInfo.u32LineCnt = stSize.u32Height / 2;
    HI_MPI_VI_SetChnLowDelayAttr(ViPipe, ViChn, &stLowDelayInfo);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("HI_MPI_VI_SetChnLowDelayAttr failed witfh %d\n", s32Ret);
        goto EXIT8;
    }

    PAUSE();

EXIT8:
    SAMPLE_COMM_VENC_StopGetStream();
EXIT7:
    SAMPLE_COMM_VPSS_UnBind_VO(VpssGrp, VpssChn, stVoConfig.VoDev, VoChn);
EXIT6:
    SAMPLE_COMM_VO_StopVO(&stVoConfig);
EXIT5:
    SAMPLE_COMM_VPSS_UnBind_VENC(VpssGrp, VpssChn, VencChn[0]);
EXIT4:
    SAMPLE_COMM_VENC_Stop(VencChn[0]);
EXIT3:
    SAMPLE_COMM_VI_UnBind_VPSS(ViPipe, ViChn, VpssGrp);
EXIT2:
    SAMPLE_COMM_VPSS_Stop(VpssGrp, abChnEnable);
EXIT1:
    SAMPLE_COMM_VI_StopVi(&stViConfig);
EXIT:
    SAMPLE_COMM_SYS_Exit();
    return s32Ret;
}

HI_S32 SAMPLE_VIO_ViRotate(HI_U32 u32VoIntfType)
{
    HI_S32             s32Ret;

    HI_S32             s32ViCnt       = 1;
    VI_DEV             ViDev          = 0;
    VI_PIPE            ViPipe         = 0;
    VI_CHN             ViChn          = 0;
    HI_S32             s32WorkSnsId   = 0;
    SAMPLE_VI_CONFIG_S stViConfig;

    SIZE_S             stSize;
    VB_CONFIG_S        stVbConf;
    PIC_SIZE_E         enPicSize;
    HI_U32             u32BlkSize;

    VO_CHN             VoChn          = 0;
    SAMPLE_VO_CONFIG_S stVoConfig;

    WDR_MODE_E         enWDRMode      = WDR_MODE_NONE;
    DYNAMIC_RANGE_E    enDynamicRange = DYNAMIC_RANGE_SDR8;
    PIXEL_FORMAT_E     enPixFormat    = PIXEL_FORMAT_YVU_SEMIPLANAR_420;
    VIDEO_FORMAT_E     enVideoFormat  = VIDEO_FORMAT_LINEAR;
    COMPRESS_MODE_E    enCompressMode = COMPRESS_MODE_NONE;

    /*config vi*/
    SAMPLE_COMM_VI_GetSensorInfo(&stViConfig);

    stViConfig.s32WorkingViNum                                   = s32ViCnt;
    stViConfig.as32WorkingViId[0]                                = 0;
    stViConfig.astViInfo[s32WorkSnsId].stSnsInfo.MipiDev         = ViDev;
    stViConfig.astViInfo[s32WorkSnsId].stDevInfo.ViDev           = ViDev;
    stViConfig.astViInfo[s32WorkSnsId].stDevInfo.enWDRMode       = enWDRMode;
    stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.enMastPipeMode = VI_OFFLINE_VPSS_OFFLINE;
    stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.aPipe[0]       = ViPipe;
    stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.aPipe[1]       = -1;
    stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.aPipe[2]       = -1;
    stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.aPipe[3]       = -1;
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.ViChn           = ViChn;
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.enPixFormat     = enPixFormat;
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.enDynamicRange  = enDynamicRange;
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.enVideoFormat   = enVideoFormat;
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.enCompressMode  = enCompressMode;

    /*get picture size*/
    s32Ret = SAMPLE_COMM_VI_GetSizeBySensor(stViConfig.astViInfo[s32WorkSnsId].stSnsInfo.enSnsType, &enPicSize);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("get picture size by sensor failed!\n");
        return s32Ret;
    }

    s32Ret = SAMPLE_COMM_SYS_GetPicSize(enPicSize, &stSize);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("get picture size failed!\n");
        return s32Ret;
    }

    /*config vb*/
    hi_memset(&stVbConf, sizeof(VB_CONFIG_S), 0, sizeof(VB_CONFIG_S));
    stVbConf.u32MaxPoolCnt              = 2;

    u32BlkSize = COMMON_GetPicBufferSize(stSize.u32Width, stSize.u32Height, SAMPLE_PIXEL_FORMAT, DATA_BITWIDTH_8, COMPRESS_MODE_SEG, DEFAULT_ALIGN);
    stVbConf.astCommPool[0].u64BlkSize  = u32BlkSize;
    stVbConf.astCommPool[0].u32BlkCnt   = 10;

    u32BlkSize = VI_GetRawBufferSize(stSize.u32Width, stSize.u32Height, PIXEL_FORMAT_RGB_BAYER_16BPP, COMPRESS_MODE_NONE, DEFAULT_ALIGN);
    stVbConf.astCommPool[1].u64BlkSize  = u32BlkSize;
    stVbConf.astCommPool[1].u32BlkCnt   = 4;

    s32Ret = SAMPLE_COMM_SYS_Init(&stVbConf);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("system init failed with %d!\n", s32Ret);
        return s32Ret;
    }

    /*start vi*/
    s32Ret = SAMPLE_COMM_VI_StartVi(&stViConfig);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("start vi failed.s32Ret:0x%x !\n", s32Ret);
        goto EXIT;
    }

    /*config vo*/
    SAMPLE_COMM_VO_GetDefConfig(&stVoConfig);
    stVoConfig.enDstDynamicRange = enDynamicRange;
    if (1 == u32VoIntfType)
    {
        stVoConfig.enVoIntfType = VO_INTF_BT1120;
    }
    else
    {
        stVoConfig.enVoIntfType = VO_INTF_HDMI;
    }
    stVoConfig.enPicSize = enPicSize;

    /*start vo*/
    s32Ret = SAMPLE_COMM_VO_StartVO(&stVoConfig);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("start vo failed. s32Ret: 0x%x !\n", s32Ret);
        goto EXIT1;
    }

    /*vi bind vo*/
    s32Ret = SAMPLE_COMM_VI_Bind_VO(ViPipe, ViChn, stVoConfig.VoDev, VoChn);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("vo bind vpss failed. s32Ret: 0x%x !\n", s32Ret);
        goto EXIT2;
    }


    printf("Press Enter key to switch Rotation 90!\n");
    getchar();

    s32Ret = HI_MPI_VI_SetChnRotation(ViPipe, ViChn, ROTATION_90);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("HI_MPI_VI_SetChnRotation failed witfh %d\n", s32Ret);
        goto EXIT3;
    }

    printf("Press Enter key to switch Rotation 180!\n");
    getchar();

    s32Ret = HI_MPI_VI_SetChnRotation(ViPipe, ViChn, ROTATION_180);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("HI_MPI_VI_SetChnRotation failed witfh %d\n", s32Ret);
        goto EXIT3;
    }

    printf("Press Enter key to switch Rotation 270!\n");
    getchar();

    s32Ret = HI_MPI_VI_SetChnRotation(ViPipe, ViChn, ROTATION_270);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("HI_MPI_VI_SetChnRotation failed witfh %d\n", s32Ret);
        goto EXIT3;
    }

    printf("Press Enter key to switch Rotation 0!\n");
    getchar();

    s32Ret = HI_MPI_VI_SetChnRotation(ViPipe, ViChn, ROTATION_0);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("HI_MPI_VI_SetChnRotation failed witfh %d\n", s32Ret);
        goto EXIT3;
    }

    PAUSE();

EXIT3:
    SAMPLE_COMM_VI_UnBind_VO(ViPipe, ViChn, stVoConfig.VoDev, VoChn);
EXIT2:
    SAMPLE_COMM_VO_StopVO(&stVoConfig);
EXIT1:
    SAMPLE_COMM_VI_StopVi(&stViConfig);
EXIT:
    SAMPLE_COMM_SYS_Exit();
    return s32Ret;
}

HI_S32 SAMPLE_VIO_ViFPN(HI_U32 u32VoIntfType)
{
    HI_S32             s32Ret;

    HI_S32             s32ViCnt       = 1;
    VI_DEV             ViDev          = 0;
    VI_PIPE            ViPipe         = 0;
    VI_CHN             ViChn          = 0;
    HI_S32             s32WorkSnsId   = 0;
    SAMPLE_VI_CONFIG_S stViConfig;

    SIZE_S             stSize;
    VB_CONFIG_S        stVbConf;
    PIC_SIZE_E         enPicSize;
    HI_U32             u32BlkSize;

    VO_CHN             VoChn          = 0;
    SAMPLE_VO_CONFIG_S stVoConfig;

    WDR_MODE_E         enWDRMode      = WDR_MODE_NONE;
    DYNAMIC_RANGE_E    enDynamicRange = DYNAMIC_RANGE_SDR8;
    PIXEL_FORMAT_E     enPixFormat    = PIXEL_FORMAT_YVU_SEMIPLANAR_420;
    VIDEO_FORMAT_E     enVideoFormat  = VIDEO_FORMAT_LINEAR;
    COMPRESS_MODE_E    enCompressMode = COMPRESS_MODE_NONE;

    SAMPLE_VI_FPN_CALIBRATE_INFO_S stViFpnCalibrateInfo;
    SAMPLE_VI_FPN_CORRECTION_INFO_S stViFpnCorrectionInfo;

    /*config vi*/
    SAMPLE_COMM_VI_GetSensorInfo(&stViConfig);

    stViConfig.s32WorkingViNum                                   = s32ViCnt;
    stViConfig.as32WorkingViId[0]                                = 0;
    stViConfig.astViInfo[s32WorkSnsId].stSnsInfo.MipiDev         = ViDev;
    stViConfig.astViInfo[s32WorkSnsId].stDevInfo.ViDev           = ViDev;
    stViConfig.astViInfo[s32WorkSnsId].stDevInfo.enWDRMode       = enWDRMode;
    stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.enMastPipeMode = VI_ONLINE_VPSS_OFFLINE;
    stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.aPipe[0]       = ViPipe;
    stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.aPipe[1]       = -1;
    stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.aPipe[2]       = -1;
    stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.aPipe[3]       = -1;
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.ViChn           = ViChn;
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.enPixFormat     = enPixFormat;
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.enDynamicRange  = enDynamicRange;
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.enVideoFormat   = enVideoFormat;
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.enCompressMode  = enCompressMode;

    /*get picture size*/
    s32Ret = SAMPLE_COMM_VI_GetSizeBySensor(stViConfig.astViInfo[s32WorkSnsId].stSnsInfo.enSnsType, &enPicSize);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("get picture size by sensor failed!\n");
        return s32Ret;
    }

    s32Ret = SAMPLE_COMM_SYS_GetPicSize(enPicSize, &stSize);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("get picture size failed!\n");
        return s32Ret;
    }

    /*config vb*/
    hi_memset(&stVbConf, sizeof(VB_CONFIG_S), 0, sizeof(VB_CONFIG_S));
    stVbConf.u32MaxPoolCnt              = 2;

    u32BlkSize = COMMON_GetPicBufferSize(stSize.u32Width, stSize.u32Height, SAMPLE_PIXEL_FORMAT, DATA_BITWIDTH_8, COMPRESS_MODE_SEG, DEFAULT_ALIGN);
    stVbConf.astCommPool[0].u64BlkSize  = u32BlkSize;
    stVbConf.astCommPool[0].u32BlkCnt   = 10;

    u32BlkSize = VI_GetRawBufferSize(stSize.u32Width, stSize.u32Height, PIXEL_FORMAT_RGB_BAYER_16BPP, COMPRESS_MODE_NONE, DEFAULT_ALIGN);
    stVbConf.astCommPool[1].u64BlkSize  = u32BlkSize;
    stVbConf.astCommPool[1].u32BlkCnt   = 4;

    s32Ret = SAMPLE_COMM_SYS_Init(&stVbConf);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("system init failed with %d!\n", s32Ret);
        return s32Ret;
    }

    /*start vi*/
    s32Ret = SAMPLE_COMM_VI_StartVi(&stViConfig);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("start vi failed.s32Ret:0x%x !\n", s32Ret);
        goto EXIT;
    }

    /*config vo*/
    SAMPLE_COMM_VO_GetDefConfig(&stVoConfig);
    stVoConfig.enDstDynamicRange = enDynamicRange;
    if (1 == u32VoIntfType)
    {
        stVoConfig.enVoIntfType = VO_INTF_BT1120;
    }
    else
    {
        stVoConfig.enVoIntfType = VO_INTF_HDMI;
    }
    stVoConfig.enPicSize = enPicSize;

    /*start vo*/
    s32Ret = SAMPLE_COMM_VO_StartVO(&stVoConfig);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("start vo failed. s32Ret: 0x%x !\n", s32Ret);
        goto EXIT1;
    }

    /*vi bind vo*/
    s32Ret = SAMPLE_COMM_VI_Bind_VO(ViPipe, ViChn, stVoConfig.VoDev, VoChn);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("vo bind vpss failed. s32Ret: 0x%x !\n", s32Ret);
        goto EXIT2;
    }

    stViFpnCalibrateInfo.u32Threshold   = 4095;
    stViFpnCalibrateInfo.u32FrameNum    = 16;
    stViFpnCalibrateInfo.enFpnType      = ISP_FPN_TYPE_FRAME;
    stViFpnCalibrateInfo.enPixelFormat  = PIXEL_FORMAT_RGB_BAYER_16BPP;
    stViFpnCalibrateInfo.enCompressMode = COMPRESS_MODE_NONE;

    SAMPLE_COMM_VI_FpnCalibrateConfig(ViPipe, &stViFpnCalibrateInfo);

    printf("Press Enter key to FPN correction!\n");
    getchar();

    stViFpnCorrectionInfo.enOpType       = OP_TYPE_AUTO;
    stViFpnCorrectionInfo.enFpnType      = stViFpnCalibrateInfo.enFpnType;
    stViFpnCorrectionInfo.u32Strength    = 0;
    stViFpnCorrectionInfo.enPixelFormat  = stViFpnCalibrateInfo.enPixelFormat;
    stViFpnCorrectionInfo.enCompressMode = stViFpnCalibrateInfo.enCompressMode;

    SAMPLE_COMM_VI_FpnCorrectionConfig(ViPipe, &stViFpnCorrectionInfo);

    PAUSE();

    SAMPLE_COMM_VI_DisableFpnCorrection(ViPipe, &stViFpnCorrectionInfo);

    SAMPLE_COMM_VI_UnBind_VO(ViPipe, ViChn, stVoConfig.VoDev, VoChn);

EXIT2:
    SAMPLE_COMM_VO_StopVO(&stVoConfig);
EXIT1:
    SAMPLE_COMM_VI_StopVi(&stViConfig);
EXIT:
    SAMPLE_COMM_SYS_Exit();
    return s32Ret;
}

HI_S32 SAMPLE_VIO_ViResoSwitch(HI_U32 u32VoIntfType)
{
    HI_S32             s32Ret;

    HI_S32             s32ViCnt       = 1;
    VI_DEV             ViDev          = 0;
    VI_PIPE            ViPipe         = 0;
    VI_CHN             ViChn          = 0;
    HI_S32             s32WorkSnsId   = 0;
    SAMPLE_VI_CONFIG_S stViConfig;

    SIZE_S             stSize;
    VB_CONFIG_S        stVbConf;
    PIC_SIZE_E         enPicSize;
    HI_U32             u32BlkSize;

    VO_CHN             VoChn          = 0;
    SAMPLE_VO_CONFIG_S stVoConfig;

    WDR_MODE_E         enWDRMode      = WDR_MODE_NONE;
    DYNAMIC_RANGE_E    enDynamicRange = DYNAMIC_RANGE_SDR8;
    PIXEL_FORMAT_E     enPixFormat    = PIXEL_FORMAT_YVU_SEMIPLANAR_420;
    VIDEO_FORMAT_E     enVideoFormat  = VIDEO_FORMAT_LINEAR;
    COMPRESS_MODE_E    enCompressMode = COMPRESS_MODE_NONE;

    VPSS_GRP           VpssGrp        = 0;
    VPSS_GRP_ATTR_S    stVpssGrpAttr;
    VPSS_CHN           VpssChn        = VPSS_CHN0;
    HI_BOOL            abChnEnable[VPSS_MAX_PHY_CHN_NUM] = {0};
    VPSS_CHN_ATTR_S    astVpssChnAttr[VPSS_MAX_PHY_CHN_NUM];

    /*config vi*/
    SAMPLE_COMM_VI_GetSensorInfo(&stViConfig);

    stViConfig.s32WorkingViNum                                   = s32ViCnt;
    stViConfig.as32WorkingViId[0]                                = 0;
    stViConfig.astViInfo[s32WorkSnsId].stSnsInfo.MipiDev         = ViDev;
    stViConfig.astViInfo[s32WorkSnsId].stDevInfo.ViDev           = ViDev;
    stViConfig.astViInfo[s32WorkSnsId].stDevInfo.enWDRMode       = enWDRMode;
    stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.enMastPipeMode = VI_ONLINE_VPSS_OFFLINE;
    stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.aPipe[0]       = ViPipe;
    stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.aPipe[1]       = -1;
    stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.aPipe[2]       = -1;
    stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.aPipe[3]       = -1;
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.ViChn           = ViChn;
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.enPixFormat     = enPixFormat;
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.enDynamicRange  = enDynamicRange;
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.enVideoFormat   = enVideoFormat;
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.enCompressMode  = enCompressMode;

    if ((stViConfig.astViInfo[s32WorkSnsId].stSnsInfo.enSnsType == SONY_IMX290_MIPI_2M_30FPS_12BIT)
        || (stViConfig.astViInfo[s32WorkSnsId].stSnsInfo.enSnsType == SONY_IMX334_MIPI_8M_30FPS_12BIT)
        || (stViConfig.astViInfo[s32WorkSnsId].stSnsInfo.enSnsType == SONY_IMX377_MIPI_8M_30FPS_10BIT))
    {
        SAMPLE_PRT("Not Support!\n");
        return HI_SUCCESS;
    }

    /*get picture size*/
    s32Ret = SAMPLE_COMM_VI_GetSizeBySensor(stViConfig.astViInfo[s32WorkSnsId].stSnsInfo.enSnsType, &enPicSize);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("get picture size by sensor failed!\n");
        return s32Ret;
    }

    s32Ret = SAMPLE_COMM_SYS_GetPicSize(enPicSize, &stSize);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("get picture size failed!\n");
        return s32Ret;
    }

    /*config vb*/
    hi_memset(&stVbConf, sizeof(VB_CONFIG_S), 0, sizeof(VB_CONFIG_S));
    stVbConf.u32MaxPoolCnt              = 2;

    u32BlkSize = COMMON_GetPicBufferSize(stSize.u32Width, stSize.u32Height, SAMPLE_PIXEL_FORMAT, DATA_BITWIDTH_8, COMPRESS_MODE_SEG, DEFAULT_ALIGN);
    stVbConf.astCommPool[0].u64BlkSize  = u32BlkSize;
    stVbConf.astCommPool[0].u32BlkCnt   = 10;

    u32BlkSize = VI_GetRawBufferSize(stSize.u32Width, stSize.u32Height, PIXEL_FORMAT_RGB_BAYER_16BPP, COMPRESS_MODE_NONE, DEFAULT_ALIGN);
    stVbConf.astCommPool[1].u64BlkSize  = u32BlkSize;
    stVbConf.astCommPool[1].u32BlkCnt   = 4;

    s32Ret = SAMPLE_COMM_SYS_Init(&stVbConf);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("system init failed with %d!\n", s32Ret);
        return s32Ret;
    }

    /*start vi*/
    s32Ret = SAMPLE_COMM_VI_StartVi(&stViConfig);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("start vi failed.s32Ret:0x%x !\n", s32Ret);
        goto EXIT;
    }

    /*config vpss*/
    hi_memset(&stVpssGrpAttr, sizeof(VPSS_GRP_ATTR_S), 0, sizeof(VPSS_GRP_ATTR_S));
    stVpssGrpAttr.stFrameRate.s32SrcFrameRate    = -1;
    stVpssGrpAttr.stFrameRate.s32DstFrameRate    = -1;
    stVpssGrpAttr.enDynamicRange                 = DYNAMIC_RANGE_SDR8;
    stVpssGrpAttr.enPixelFormat                  = enPixFormat;
    stVpssGrpAttr.u32MaxW                        = stSize.u32Width;
    stVpssGrpAttr.u32MaxH                        = stSize.u32Height;
    stVpssGrpAttr.bNrEn                          = HI_TRUE;
    stVpssGrpAttr.stNrAttr.enCompressMode        = COMPRESS_MODE_FRAME;
    stVpssGrpAttr.stNrAttr.enNrMotionMode        = NR_MOTION_MODE_NORMAL;

    astVpssChnAttr[VpssChn].u32Width                    = stSize.u32Width;
    astVpssChnAttr[VpssChn].u32Height                   = stSize.u32Height;
    astVpssChnAttr[VpssChn].enChnMode                   = VPSS_CHN_MODE_USER;
    astVpssChnAttr[VpssChn].enCompressMode              = enCompressMode;
    astVpssChnAttr[VpssChn].enDynamicRange              = enDynamicRange;
    astVpssChnAttr[VpssChn].enVideoFormat               = enVideoFormat;
    astVpssChnAttr[VpssChn].enPixelFormat               = enPixFormat;
    astVpssChnAttr[VpssChn].stFrameRate.s32SrcFrameRate = 30;
    astVpssChnAttr[VpssChn].stFrameRate.s32DstFrameRate = 30;
    astVpssChnAttr[VpssChn].u32Depth                    = 0;
    astVpssChnAttr[VpssChn].bMirror                     = HI_FALSE;
    astVpssChnAttr[VpssChn].bFlip                       = HI_FALSE;
    astVpssChnAttr[VpssChn].stAspectRatio.enMode        = ASPECT_RATIO_NONE;

    /*start vpss*/
    abChnEnable[0] = HI_TRUE;
    s32Ret = SAMPLE_COMM_VPSS_Start(VpssGrp, abChnEnable, &stVpssGrpAttr, astVpssChnAttr);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("start vpss group failed. s32Ret: 0x%x !\n", s32Ret);
        goto EXIT1;
    }

    /*vi bind vpss*/
    s32Ret = SAMPLE_COMM_VI_Bind_VPSS(ViPipe, ViChn, VpssGrp);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("vi bind vpss failed. s32Ret: 0x%x !\n", s32Ret);
        goto EXIT2;
    }

    /*config vo*/
    SAMPLE_COMM_VO_GetDefConfig(&stVoConfig);
    stVoConfig.enDstDynamicRange = enDynamicRange;
    if (1 == u32VoIntfType)
    {
        stVoConfig.enVoIntfType = VO_INTF_BT1120;
    }
    else
    {
        stVoConfig.enVoIntfType = VO_INTF_HDMI;
    }
    stVoConfig.enPicSize = enPicSize;

    /*start vo*/
    s32Ret = SAMPLE_COMM_VO_StartVO(&stVoConfig);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("start vo failed. s32Ret: 0x%x !\n", s32Ret);
        goto EXIT3;
    }

    /*vpss bind vo*/
    s32Ret = SAMPLE_COMM_VPSS_Bind_VO(VpssGrp, VpssChn, stVoConfig.VoDev, VoChn);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("vo bind vpss failed. s32Ret: 0x%x !\n", s32Ret);
        goto EXIT4;
    }

    printf("switch to 720P 120fps========\n");
    getchar();

    SAMPLE_COMM_VPSS_Stop(VpssGrp, abChnEnable);
    SAMPLE_COMM_VI_SwitchMode_StopVI(&stViConfig);

    stViConfig.astViInfo[s32WorkSnsId].stSnsInfo.enSnsType = SONY_IMX290_MIPI_2M_30FPS_12BIT;

    SAMPLE_COMM_VI_SwitchMode(&stViConfig);


    stVpssGrpAttr.u32MaxW                        = 1280;
    stVpssGrpAttr.u32MaxH                        = 720;

    astVpssChnAttr[VpssChn].u32Width             = 1280;
    astVpssChnAttr[VpssChn].u32Height            = 720;

    s32Ret = SAMPLE_COMM_VPSS_Start(VpssGrp, abChnEnable, &stVpssGrpAttr, astVpssChnAttr);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("start vpss group failed. s32Ret: 0x%x !\n", s32Ret);
        goto EXIT1;
    }

    printf("switch to 4K 30fps========\n");
    getchar();

    SAMPLE_COMM_VPSS_Stop(VpssGrp, abChnEnable);
    SAMPLE_COMM_VI_SwitchMode_StopVI(&stViConfig);

    stViConfig.astViInfo[s32WorkSnsId].stSnsInfo.enSnsType = SENSOR0_TYPE;

    SAMPLE_COMM_VI_SwitchMode(&stViConfig);

    stVpssGrpAttr.u32MaxW                        = stSize.u32Width;
    stVpssGrpAttr.u32MaxH                        = stSize.u32Height;

    astVpssChnAttr[VpssChn].u32Width             = stSize.u32Width;
    astVpssChnAttr[VpssChn].u32Height            = stSize.u32Height;

    s32Ret = SAMPLE_COMM_VPSS_Start(VpssGrp, abChnEnable, &stVpssGrpAttr, astVpssChnAttr);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("start vpss group failed. s32Ret: 0x%x !\n", s32Ret);
        goto EXIT1;
    }

    PAUSE();

    SAMPLE_COMM_VPSS_UnBind_VO(VpssGrp, VpssChn, stVoConfig.VoDev, VoChn);

EXIT4:
    SAMPLE_COMM_VO_StopVO(&stVoConfig);
EXIT3:
    SAMPLE_COMM_VI_UnBind_VPSS(ViPipe, ViChn, VpssGrp);
EXIT2:
    SAMPLE_COMM_VPSS_Stop(VpssGrp, abChnEnable);
EXIT1:
    SAMPLE_COMM_VI_StopVi(&stViConfig);
EXIT:
    SAMPLE_COMM_SYS_Exit();
    return s32Ret;
}

HI_S32 SAMPLE_VIO_ViDoubleWdrPipe(HI_U32 u32VoIntfType)
{
    HI_S32             s32Ret;

    HI_S32             s32ViCnt       = 2;
    VI_DEV             ViDev[2]       = {0, 2};
    VI_PIPE            ViPipe[4]      = {0, 1, 2, 3};
    VI_CHN             ViChn          = 0;
    SAMPLE_VI_CONFIG_S stViConfig;

    SIZE_S             stSize;
    VB_CONFIG_S        stVbConf;
    PIC_SIZE_E         enPicSize;
    HI_U32             u32BlkSize;

    VO_CHN             VoChn          = 0;
    SAMPLE_VO_CONFIG_S stVoConfig;

    WDR_MODE_E         enWDRMode      = WDR_MODE_2To1_LINE;
    DYNAMIC_RANGE_E    enDynamicRange = DYNAMIC_RANGE_SDR8;
    PIXEL_FORMAT_E     enPixFormat    = PIXEL_FORMAT_YVU_SEMIPLANAR_420;
    VIDEO_FORMAT_E     enVideoFormat  = VIDEO_FORMAT_LINEAR;
    COMPRESS_MODE_E    enCompressMode = COMPRESS_MODE_NONE;

    VPSS_GRP           VpssGrp[2]     = {0, 1};
    VPSS_GRP_ATTR_S    stVpssGrpAttr;
    VPSS_CHN           VpssChn        = VPSS_CHN0;
    HI_BOOL            abChnEnable[VPSS_MAX_PHY_CHN_NUM] = {0};
    VPSS_CHN_ATTR_S    astVpssChnAttr[VPSS_MAX_PHY_CHN_NUM];

    /*config vi*/
    SAMPLE_COMM_VI_GetSensorInfo(&stViConfig);

    stViConfig.s32WorkingViNum = s32ViCnt;
    stViConfig.as32WorkingViId[0] = 0;
    stViConfig.as32WorkingViId[1] = 1;
    stViConfig.astViInfo[0].stSnsInfo.MipiDev         = ViDev[0];
    stViConfig.astViInfo[0].stSnsInfo.s32BusId        = 0;
    stViConfig.astViInfo[0].stDevInfo.ViDev           = ViDev[0];
    stViConfig.astViInfo[0].stDevInfo.enWDRMode       = enWDRMode;
    stViConfig.astViInfo[0].stPipeInfo.enMastPipeMode = VI_OFFLINE_VPSS_OFFLINE;
    stViConfig.astViInfo[0].stPipeInfo.aPipe[0]       = ViPipe[0];
    stViConfig.astViInfo[0].stPipeInfo.aPipe[1]       = ViPipe[1];
    stViConfig.astViInfo[0].stPipeInfo.aPipe[2]       = -1;
    stViConfig.astViInfo[0].stPipeInfo.aPipe[3]       = -1;
    stViConfig.astViInfo[0].stChnInfo.ViChn           = ViChn;
    stViConfig.astViInfo[0].stChnInfo.enPixFormat     = enPixFormat;
    stViConfig.astViInfo[0].stChnInfo.enDynamicRange  = enDynamicRange;
    stViConfig.astViInfo[0].stChnInfo.enVideoFormat   = enVideoFormat;
    stViConfig.astViInfo[0].stChnInfo.enCompressMode  = enCompressMode;

    stViConfig.astViInfo[1].stSnsInfo.MipiDev         = ViDev[1];
    stViConfig.astViInfo[1].stSnsInfo.s32BusId        = 1;
    stViConfig.astViInfo[1].stDevInfo.ViDev           = ViDev[1];
    stViConfig.astViInfo[1].stDevInfo.enWDRMode       = enWDRMode;
    stViConfig.astViInfo[1].stPipeInfo.enMastPipeMode = VI_OFFLINE_VPSS_OFFLINE;
    stViConfig.astViInfo[1].stPipeInfo.aPipe[0]       = ViPipe[2];
    stViConfig.astViInfo[1].stPipeInfo.aPipe[1]       = ViPipe[3];
    stViConfig.astViInfo[1].stPipeInfo.aPipe[2]       = -1;
    stViConfig.astViInfo[1].stPipeInfo.aPipe[3]       = -1;
    stViConfig.astViInfo[1].stChnInfo.ViChn           = ViChn;
    stViConfig.astViInfo[1].stChnInfo.enPixFormat     = enPixFormat;
    stViConfig.astViInfo[1].stChnInfo.enDynamicRange  = enDynamicRange;
    stViConfig.astViInfo[1].stChnInfo.enVideoFormat   = enVideoFormat;
    stViConfig.astViInfo[1].stChnInfo.enCompressMode  = enCompressMode;

    if ((stViConfig.astViInfo[0].stSnsInfo.enSnsType == SONY_IMX290_MIPI_2M_30FPS_12BIT)
        || (stViConfig.astViInfo[0].stSnsInfo.enSnsType == SONY_IMX334_MIPI_8M_30FPS_12BIT)
        || (stViConfig.astViInfo[0].stSnsInfo.enSnsType == SONY_IMX377_MIPI_8M_30FPS_10BIT))
    {
        SAMPLE_PRT("Not Support!\n");
        return HI_SUCCESS;
    }

    /*get picture size*/
    s32Ret = SAMPLE_COMM_VI_GetSizeBySensor(stViConfig.astViInfo[0].stSnsInfo.enSnsType, &enPicSize);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("get picture size by sensor failed!\n");
        return s32Ret;
    }

    s32Ret = SAMPLE_COMM_SYS_GetPicSize(enPicSize, &stSize);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("get picture size failed!\n");
        return s32Ret;
    }

    /*config vb*/
    hi_memset(&stVbConf, sizeof(VB_CONFIG_S), 0, sizeof(VB_CONFIG_S));
    stVbConf.u32MaxPoolCnt              = 2;

    u32BlkSize = COMMON_GetPicBufferSize(stSize.u32Width, stSize.u32Height, SAMPLE_PIXEL_FORMAT, DATA_BITWIDTH_8, COMPRESS_MODE_SEG, DEFAULT_ALIGN);
    stVbConf.astCommPool[0].u64BlkSize  = u32BlkSize;
    stVbConf.astCommPool[0].u32BlkCnt   = 20;

    u32BlkSize = VI_GetRawBufferSize(stSize.u32Width, stSize.u32Height, PIXEL_FORMAT_RGB_BAYER_16BPP, COMPRESS_MODE_NONE, DEFAULT_ALIGN);
    stVbConf.astCommPool[1].u64BlkSize  = u32BlkSize;
    stVbConf.astCommPool[1].u32BlkCnt   = 4;

    s32Ret = SAMPLE_COMM_SYS_Init(&stVbConf);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("system init failed with %d!\n", s32Ret);
        return s32Ret;
    }

    /*start vi*/
    s32Ret = SAMPLE_COMM_VI_StartVi(&stViConfig);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("start vi failed.s32Ret:0x%x !\n", s32Ret);
        goto EXIT;
    }

    /*config vpss*/
    hi_memset(&stVpssGrpAttr, sizeof(VPSS_GRP_ATTR_S), 0, sizeof(VPSS_GRP_ATTR_S));
    stVpssGrpAttr.stFrameRate.s32SrcFrameRate    = -1;
    stVpssGrpAttr.stFrameRate.s32DstFrameRate    = -1;
    stVpssGrpAttr.enDynamicRange                 = DYNAMIC_RANGE_SDR8;
    stVpssGrpAttr.enPixelFormat                  = enPixFormat;
    stVpssGrpAttr.u32MaxW                        = stSize.u32Width;
    stVpssGrpAttr.u32MaxH                        = stSize.u32Height;
    stVpssGrpAttr.bNrEn                          = HI_TRUE;
    stVpssGrpAttr.stNrAttr.enCompressMode        = COMPRESS_MODE_FRAME;
    stVpssGrpAttr.stNrAttr.enNrMotionMode        = NR_MOTION_MODE_NORMAL;

    astVpssChnAttr[VpssChn].u32Width                    = stSize.u32Width;
    astVpssChnAttr[VpssChn].u32Height                   = stSize.u32Height;
    astVpssChnAttr[VpssChn].enChnMode                   = VPSS_CHN_MODE_USER;
    astVpssChnAttr[VpssChn].enCompressMode              = enCompressMode;
    astVpssChnAttr[VpssChn].enDynamicRange              = enDynamicRange;
    astVpssChnAttr[VpssChn].enVideoFormat               = enVideoFormat;
    astVpssChnAttr[VpssChn].enPixelFormat               = enPixFormat;
    astVpssChnAttr[VpssChn].stFrameRate.s32SrcFrameRate = 30;
    astVpssChnAttr[VpssChn].stFrameRate.s32DstFrameRate = 30;
    astVpssChnAttr[VpssChn].u32Depth                    = 0;
    astVpssChnAttr[VpssChn].bMirror                     = HI_FALSE;
    astVpssChnAttr[VpssChn].bFlip                       = HI_FALSE;
    astVpssChnAttr[VpssChn].stAspectRatio.enMode        = ASPECT_RATIO_NONE;

    /*start vpss*/
    abChnEnable[0] = HI_TRUE;
    s32Ret = SAMPLE_COMM_VPSS_Start(VpssGrp[0], abChnEnable, &stVpssGrpAttr, astVpssChnAttr);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("start vpss group failed. s32Ret: 0x%x !\n", s32Ret);
        goto EXIT1;
    }

    s32Ret = SAMPLE_COMM_VPSS_Start(VpssGrp[1], abChnEnable, &stVpssGrpAttr, astVpssChnAttr);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("start vpss group failed. s32Ret: 0x%x !\n", s32Ret);
        goto EXIT2;
    }

    /*vi bind vpss*/
    s32Ret = SAMPLE_COMM_VI_Bind_VPSS(ViPipe[0], ViChn, VpssGrp[0]);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("vi bind vpss failed. s32Ret: 0x%x !\n", s32Ret);
        goto EXIT3;
    }

    s32Ret = SAMPLE_COMM_VI_Bind_VPSS(ViPipe[2], ViChn, VpssGrp[1]);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("vi bind vpss failed. s32Ret: 0x%x !\n", s32Ret);
        goto EXIT4;
    }

    /*config vo*/
    SAMPLE_COMM_VO_GetDefConfig(&stVoConfig);
    stVoConfig.enDstDynamicRange = enDynamicRange;
    if (1 == u32VoIntfType)
    {
        stVoConfig.enVoIntfType = VO_INTF_BT1120;
    }
    else
    {
        stVoConfig.enVoIntfType = VO_INTF_HDMI;
    }
    stVoConfig.enPicSize = enPicSize;

    /*start vo*/
    s32Ret = SAMPLE_COMM_VO_StartVO(&stVoConfig);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("start vo failed. s32Ret: 0x%x !\n", s32Ret);
        goto EXIT5;
    }

    /*vpss bind vo*/
    s32Ret = SAMPLE_COMM_VPSS_Bind_VO(VpssGrp[0], VpssChn, stVoConfig.VoDev, VoChn);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("vo bind vpss failed. s32Ret: 0x%x !\n", s32Ret);
        goto EXIT6;
    }

    PAUSE();

    SAMPLE_COMM_VPSS_UnBind_VO(VpssGrp[0], VpssChn, stVoConfig.VoDev, VoChn);

EXIT6:
    SAMPLE_COMM_VO_StopVO(&stVoConfig);
EXIT5:
    SAMPLE_COMM_VI_UnBind_VPSS(ViPipe[2], ViChn, VpssGrp[1]);
EXIT4:
    SAMPLE_COMM_VI_UnBind_VPSS(ViPipe[0], ViChn, VpssGrp[0]);
EXIT3:
    SAMPLE_COMM_VPSS_Stop(VpssGrp[1], abChnEnable);
EXIT2:
    SAMPLE_COMM_VPSS_Stop(VpssGrp[0], abChnEnable);
EXIT1:
    SAMPLE_COMM_VI_StopVi(&stViConfig);
EXIT:
    SAMPLE_COMM_SYS_Exit();
    return s32Ret;
}

HI_S32 SAMPLE_VIO_ViDeMuxYuv(HI_U32 u32VoIntfType)
{
    HI_S32             s32Ret;

    HI_S32             s32ViCnt       = 1;
    VI_DEV             ViDev          = 0;
    VI_PIPE            ViPipe[4]      = {2, 3, 4, 5};
    VI_CHN             ViChn          = 0;
    HI_S32             s32WorkSnsId   = 0;
    SAMPLE_VI_CONFIG_S stViConfig;

    SIZE_S             stSize;
    VB_CONFIG_S        stVbConf;
    PIC_SIZE_E         enPicSize;
    HI_U32             u32BlkSize;

    VO_CHN             VoChn[4]       = {0, 1, 2, 3};
    SAMPLE_VO_CONFIG_S stVoConfig;

    WDR_MODE_E         enWDRMode      = WDR_MODE_NONE;
    DYNAMIC_RANGE_E    enDynamicRange = DYNAMIC_RANGE_SDR8;
    PIXEL_FORMAT_E     enPixFormat    = PIXEL_FORMAT_YVU_SEMIPLANAR_420;
    VIDEO_FORMAT_E     enVideoFormat  = VIDEO_FORMAT_LINEAR;
    COMPRESS_MODE_E    enCompressMode = COMPRESS_MODE_NONE;

    VPSS_GRP           VpssGrp[4]     = {0, 1, 2, 3};
    VPSS_GRP_ATTR_S    stVpssGrpAttr;
    VPSS_CHN           VpssChn        = VPSS_CHN0;
    HI_BOOL            abChnEnable[VPSS_MAX_PHY_CHN_NUM] = {0};
    VPSS_CHN_ATTR_S    astVpssChnAttr[VPSS_MAX_PHY_CHN_NUM];

    /*config vi*/
    SAMPLE_COMM_VI_GetSensorInfo(&stViConfig);

    stViConfig.s32WorkingViNum                                   = s32ViCnt;
    stViConfig.as32WorkingViId[0]                                = 0;
    stViConfig.astViInfo[s32WorkSnsId].stSnsInfo.MipiDev         = ViDev;
    stViConfig.astViInfo[s32WorkSnsId].stDevInfo.ViDev           = ViDev;
    stViConfig.astViInfo[s32WorkSnsId].stDevInfo.enWDRMode       = enWDRMode;
    stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.enMastPipeMode = VI_OFFLINE_VPSS_OFFLINE;
    stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.aPipe[0]       = ViPipe[0];
    stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.aPipe[1]       = ViPipe[1];
    stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.aPipe[2]       = ViPipe[2];
    stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.aPipe[3]       = ViPipe[3];
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.ViChn           = ViChn;
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.enPixFormat     = enPixFormat;
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.enDynamicRange  = enDynamicRange;
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.enVideoFormat   = enVideoFormat;
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.enCompressMode  = enCompressMode;

    stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.bMultiPipe     = HI_TRUE;
    stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.bVcNumCfged     = HI_TRUE;
    stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.u32VCNum[0] = 0;
    stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.u32VCNum[1] = 1;
    stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.u32VCNum[2] = 2;
    stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.u32VCNum[3] = 3;

    if ((stViConfig.astViInfo[s32WorkSnsId].stSnsInfo.enSnsType == SONY_IMX290_MIPI_2M_30FPS_12BIT)
        || (stViConfig.astViInfo[s32WorkSnsId].stSnsInfo.enSnsType == SONY_IMX334_MIPI_8M_30FPS_12BIT)
        || (stViConfig.astViInfo[s32WorkSnsId].stSnsInfo.enSnsType == SONY_IMX377_MIPI_8M_30FPS_10BIT))
    {
        SAMPLE_PRT("Not Support!\n");
        return HI_SUCCESS;
    }

    /*get picture size*/
    s32Ret = SAMPLE_COMM_VI_GetSizeBySensor(stViConfig.astViInfo[s32WorkSnsId].stSnsInfo.enSnsType, &enPicSize);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("get picture size by sensor failed!\n");
        return s32Ret;
    }

    s32Ret = SAMPLE_COMM_SYS_GetPicSize(enPicSize, &stSize);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("get picture size failed!\n");
        return s32Ret;
    }

    /*config vb*/
    hi_memset(&stVbConf, sizeof(VB_CONFIG_S), 0, sizeof(VB_CONFIG_S));
    stVbConf.u32MaxPoolCnt              = 2;

    u32BlkSize = COMMON_GetPicBufferSize(stSize.u32Width, stSize.u32Height, SAMPLE_PIXEL_FORMAT, DATA_BITWIDTH_8, COMPRESS_MODE_SEG, DEFAULT_ALIGN);
    stVbConf.astCommPool[0].u64BlkSize  = u32BlkSize;
    stVbConf.astCommPool[0].u32BlkCnt   = 20;

    u32BlkSize = VI_GetRawBufferSize(stSize.u32Width, stSize.u32Height, PIXEL_FORMAT_RGB_BAYER_16BPP, COMPRESS_MODE_NONE, DEFAULT_ALIGN);
    stVbConf.astCommPool[1].u64BlkSize  = u32BlkSize;
    stVbConf.astCommPool[1].u32BlkCnt   = 4;

    s32Ret = SAMPLE_COMM_SYS_Init(&stVbConf);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("system init failed with %d!\n", s32Ret);
        return s32Ret;
    }

    /*start vi*/
    s32Ret = SAMPLE_COMM_VI_StartVi(&stViConfig);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("start vi failed.s32Ret:0x%x !\n", s32Ret);
        goto EXIT;
    }

    /*config vpss*/
    hi_memset(&stVpssGrpAttr, sizeof(VPSS_GRP_ATTR_S), 0, sizeof(VPSS_GRP_ATTR_S));
    stVpssGrpAttr.stFrameRate.s32SrcFrameRate    = -1;
    stVpssGrpAttr.stFrameRate.s32DstFrameRate    = -1;
    stVpssGrpAttr.enDynamicRange                 = DYNAMIC_RANGE_SDR8;
    stVpssGrpAttr.enPixelFormat                  = enPixFormat;
    stVpssGrpAttr.u32MaxW                        = stSize.u32Width;
    stVpssGrpAttr.u32MaxH                        = stSize.u32Height;
    stVpssGrpAttr.bNrEn                          = HI_TRUE;
    stVpssGrpAttr.stNrAttr.enCompressMode        = COMPRESS_MODE_FRAME;
    stVpssGrpAttr.stNrAttr.enNrMotionMode        = NR_MOTION_MODE_NORMAL;

    astVpssChnAttr[VpssChn].u32Width                    = stSize.u32Width;
    astVpssChnAttr[VpssChn].u32Height                   = stSize.u32Height;
    astVpssChnAttr[VpssChn].enChnMode                   = VPSS_CHN_MODE_USER;
    astVpssChnAttr[VpssChn].enCompressMode              = enCompressMode;
    astVpssChnAttr[VpssChn].enDynamicRange              = enDynamicRange;
    astVpssChnAttr[VpssChn].enVideoFormat               = enVideoFormat;
    astVpssChnAttr[VpssChn].enPixelFormat               = enPixFormat;
    astVpssChnAttr[VpssChn].stFrameRate.s32SrcFrameRate = 30;
    astVpssChnAttr[VpssChn].stFrameRate.s32DstFrameRate = 30;
    astVpssChnAttr[VpssChn].u32Depth                    = 0;
    astVpssChnAttr[VpssChn].bMirror                     = HI_FALSE;
    astVpssChnAttr[VpssChn].bFlip                       = HI_FALSE;
    astVpssChnAttr[VpssChn].stAspectRatio.enMode        = ASPECT_RATIO_NONE;

    /*start vpss*/
    abChnEnable[0] = HI_TRUE;
    s32Ret = SAMPLE_COMM_VPSS_Start(VpssGrp[0], abChnEnable, &stVpssGrpAttr, astVpssChnAttr);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("start vpss group failed. s32Ret: 0x%x !\n", s32Ret);
        goto EXIT1;
    }

    s32Ret = SAMPLE_COMM_VPSS_Start(VpssGrp[1], abChnEnable, &stVpssGrpAttr, astVpssChnAttr);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("start vpss group failed. s32Ret: 0x%x !\n", s32Ret);
        goto EXIT2;
    }

    s32Ret = SAMPLE_COMM_VPSS_Start(VpssGrp[2], abChnEnable, &stVpssGrpAttr, astVpssChnAttr);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("start vpss group failed. s32Ret: 0x%x !\n", s32Ret);
        goto EXIT2;
    }

    s32Ret = SAMPLE_COMM_VPSS_Start(VpssGrp[3], abChnEnable, &stVpssGrpAttr, astVpssChnAttr);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("start vpss group failed. s32Ret: 0x%x !\n", s32Ret);
        goto EXIT2;
    }


    /*vi bind vpss*/
    s32Ret |= SAMPLE_COMM_VI_Bind_VPSS(ViPipe[0], ViChn, VpssGrp[0]);
    s32Ret |= SAMPLE_COMM_VI_Bind_VPSS(ViPipe[1], ViChn, VpssGrp[1]);
    s32Ret |= SAMPLE_COMM_VI_Bind_VPSS(ViPipe[2], ViChn, VpssGrp[2]);
    s32Ret |= SAMPLE_COMM_VI_Bind_VPSS(ViPipe[3], ViChn, VpssGrp[3]);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("vi bind vpss failed. s32Ret: 0x%x !\n", s32Ret);
        goto EXIT3;
    }

    /*config vo*/
    SAMPLE_COMM_VO_GetDefConfig(&stVoConfig);
    stVoConfig.enVoMode = VO_MODE_4MUX;
    stVoConfig.enDstDynamicRange = enDynamicRange;
    if (1 == u32VoIntfType)
    {
        stVoConfig.enVoIntfType = VO_INTF_BT1120;
    }
    else
    {
        stVoConfig.enVoIntfType = VO_INTF_HDMI;
    }
    stVoConfig.enPicSize = enPicSize;

    /*start vo*/
    s32Ret = SAMPLE_COMM_VO_StartVO(&stVoConfig);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("start vo failed. s32Ret: 0x%x !\n", s32Ret);
        goto EXIT4;
    }

    /*vpss bind vo*/
    s32Ret |= SAMPLE_COMM_VPSS_Bind_VO(VpssGrp[0], VpssChn, stVoConfig.VoDev, VoChn[0]);
    s32Ret |= SAMPLE_COMM_VPSS_Bind_VO(VpssGrp[1], VpssChn, stVoConfig.VoDev, VoChn[1]);
    s32Ret |= SAMPLE_COMM_VPSS_Bind_VO(VpssGrp[2], VpssChn, stVoConfig.VoDev, VoChn[2]);
    s32Ret |= SAMPLE_COMM_VPSS_Bind_VO(VpssGrp[3], VpssChn, stVoConfig.VoDev, VoChn[3]);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("vo bind vpss failed. s32Ret: 0x%x !\n", s32Ret);
        goto EXIT5;
    }

    PAUSE();

    SAMPLE_COMM_VPSS_UnBind_VO(VpssGrp[0], VpssChn, stVoConfig.VoDev, VoChn[0]);
    SAMPLE_COMM_VPSS_UnBind_VO(VpssGrp[1], VpssChn, stVoConfig.VoDev, VoChn[1]);
    SAMPLE_COMM_VPSS_UnBind_VO(VpssGrp[2], VpssChn, stVoConfig.VoDev, VoChn[2]);
    SAMPLE_COMM_VPSS_UnBind_VO(VpssGrp[3], VpssChn, stVoConfig.VoDev, VoChn[3]);

EXIT5:
    SAMPLE_COMM_VO_StopVO(&stVoConfig);
EXIT4:
    SAMPLE_COMM_VI_UnBind_VPSS(ViPipe[0], ViChn, VpssGrp[0]);
    SAMPLE_COMM_VI_UnBind_VPSS(ViPipe[1], ViChn, VpssGrp[1]);
    SAMPLE_COMM_VI_UnBind_VPSS(ViPipe[2], ViChn, VpssGrp[2]);
    SAMPLE_COMM_VI_UnBind_VPSS(ViPipe[3], ViChn, VpssGrp[3]);
EXIT3:
    SAMPLE_COMM_VPSS_Stop(VpssGrp[3], abChnEnable);
    SAMPLE_COMM_VPSS_Stop(VpssGrp[2], abChnEnable);
    SAMPLE_COMM_VPSS_Stop(VpssGrp[1], abChnEnable);
EXIT2:
    SAMPLE_COMM_VPSS_Stop(VpssGrp[0], abChnEnable);
EXIT1:
    SAMPLE_COMM_VI_StopVi(&stViConfig);
EXIT:
    SAMPLE_COMM_SYS_Exit();
    return s32Ret;
}

HI_S32 SAMPLE_VIO_ViSetUsrPic(HI_U32 u32VoIntfType)
{
    HI_S32             s32Ret;

    HI_S32             s32ViCnt       = 1;
    VI_DEV             ViDev          = 0;
    VI_PIPE            ViPipe         = 0;
    VI_CHN             ViChn          = 0;
    HI_S32             s32WorkSnsId   = 0;
    SAMPLE_VI_CONFIG_S stViConfig;

    SIZE_S             stSize;
    VB_CONFIG_S        stVbConf;
    PIC_SIZE_E         enPicSize;
    HI_U32             u32BlkSize;

    VO_CHN             VoChn          = 0;
    SAMPLE_VO_CONFIG_S stVoConfig;

    WDR_MODE_E         enWDRMode      = WDR_MODE_NONE;
    DYNAMIC_RANGE_E    enDynamicRange = DYNAMIC_RANGE_SDR8;
    PIXEL_FORMAT_E     enPixFormat    = PIXEL_FORMAT_YVU_SEMIPLANAR_420;
    VIDEO_FORMAT_E     enVideoFormat  = VIDEO_FORMAT_LINEAR;
    COMPRESS_MODE_E    enCompressMode = COMPRESS_MODE_NONE;

    HI_CHAR YuvFileName[64];

    SAMPLE_VI_FRAME_INFO_S stViFrameInfo;
    VI_USERPIC_ATTR_S stUsrPic;

    /*config vi*/
    SAMPLE_COMM_VI_GetSensorInfo(&stViConfig);

    stViConfig.s32WorkingViNum                                   = s32ViCnt;
    stViConfig.as32WorkingViId[0]                                = 0;
    stViConfig.astViInfo[s32WorkSnsId].stSnsInfo.MipiDev         = ViDev;
    stViConfig.astViInfo[s32WorkSnsId].stDevInfo.ViDev           = ViDev;
    stViConfig.astViInfo[s32WorkSnsId].stDevInfo.enWDRMode       = enWDRMode;
    stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.enMastPipeMode = VI_OFFLINE_VPSS_OFFLINE;
    stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.aPipe[0]       = ViPipe;
    stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.aPipe[1]       = -1;
    stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.aPipe[2]       = -1;
    stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.aPipe[3]       = -1;
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.ViChn           = ViChn;
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.enPixFormat     = enPixFormat;
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.enDynamicRange  = enDynamicRange;
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.enVideoFormat   = enVideoFormat;
    stViConfig.astViInfo[s32WorkSnsId].stChnInfo.enCompressMode  = enCompressMode;

    if (stViConfig.astViInfo[s32WorkSnsId].stSnsInfo.enSnsType == SONY_IMX290_MIPI_2M_30FPS_12BIT)
    {
        snprintf(YuvFileName, sizeof(YuvFileName), "%s", "./res/UsePic_1920_1080_420.yuv");
    }
    else
    {
        snprintf(YuvFileName, sizeof(YuvFileName), "%s", "./res/UsePic_3840_2160_420.yuv");
    }

    /*get picture size*/
    s32Ret = SAMPLE_COMM_VI_GetSizeBySensor(stViConfig.astViInfo[s32WorkSnsId].stSnsInfo.enSnsType, &enPicSize);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("get picture size by sensor failed!\n");
        return s32Ret;
    }

    s32Ret = SAMPLE_COMM_SYS_GetPicSize(enPicSize, &stSize);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("get picture size failed!\n");
        return s32Ret;
    }

    /*config vb*/
    hi_memset(&stVbConf, sizeof(VB_CONFIG_S), 0, sizeof(VB_CONFIG_S));
    stVbConf.u32MaxPoolCnt              = 2;

    u32BlkSize = COMMON_GetPicBufferSize(stSize.u32Width, stSize.u32Height, SAMPLE_PIXEL_FORMAT, DATA_BITWIDTH_8, COMPRESS_MODE_SEG, DEFAULT_ALIGN);
    stVbConf.astCommPool[0].u64BlkSize  = u32BlkSize;
    stVbConf.astCommPool[0].u32BlkCnt   = 10;

    u32BlkSize = VI_GetRawBufferSize(stSize.u32Width, stSize.u32Height, PIXEL_FORMAT_RGB_BAYER_16BPP, COMPRESS_MODE_NONE, DEFAULT_ALIGN);
    stVbConf.astCommPool[1].u64BlkSize  = u32BlkSize;
    stVbConf.astCommPool[1].u32BlkCnt   = 4;

    s32Ret = SAMPLE_COMM_SYS_Init(&stVbConf);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("system init failed with %d!\n", s32Ret);
        return s32Ret;
    }

    /*start vi*/
    s32Ret = SAMPLE_COMM_VI_StartVi(&stViConfig);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("start vi failed.s32Ret:0x%x !\n", s32Ret);
        goto EXIT;
    }

    /*config vo*/
    SAMPLE_COMM_VO_GetDefConfig(&stVoConfig);
    stVoConfig.enDstDynamicRange = enDynamicRange;
    if (1 == u32VoIntfType)
    {
        stVoConfig.enVoIntfType = VO_INTF_BT1120;
    }
    else
    {
        stVoConfig.enVoIntfType = VO_INTF_HDMI;
    }
    stVoConfig.enPicSize = enPicSize;

    /*start vo*/
    s32Ret = SAMPLE_COMM_VO_StartVO(&stVoConfig);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("start vo failed. s32Ret: 0x%x !\n", s32Ret);
        goto EXIT1;
    }

    /*vi bind vo*/
    s32Ret = SAMPLE_COMM_VI_Bind_VO(ViPipe, ViChn, stVoConfig.VoDev, VoChn);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("vo bind vpss failed. s32Ret: 0x%x !\n", s32Ret);
        goto EXIT2;
    }

    printf("Press Enter key to Enable Normal UsePic!\n");
    getchar();

    stUsrPic.enUsrPicMode = VI_USERPIC_MODE_PIC;
    stUsrPic.unUsrPic.stUsrPicFrm.stVFrame.u32Width = stSize.u32Width;
    stUsrPic.unUsrPic.stUsrPicFrm.stVFrame.u32Height = stSize.u32Height;
    stUsrPic.unUsrPic.stUsrPicFrm.stVFrame.enPixelFormat = PIXEL_FORMAT_YVU_SEMIPLANAR_420;

    s32Ret = SAMPLE_COMM_VI_Load_UserPic(YuvFileName, &stUsrPic, &stViFrameInfo);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_VI_Load_UserPic failed with %#x!\n", s32Ret);
        goto EXIT3;
    }

    s32Ret = HI_MPI_VI_SetUserPic(ViPipe, &stUsrPic);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("HI_MPI_VI_SetUserPic failed with %#x!\n", s32Ret);
        goto EXIT4;
    }

    s32Ret = HI_MPI_VI_EnableUserPic(ViPipe);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("HI_MPI_VI_EnableUserPic failed with %#x!\n", s32Ret);
        goto EXIT4;
    }

    printf("Press Enter key to Enable GBC UsePic!\n");
    getchar();
    stUsrPic.enUsrPicMode = VI_USERPIC_MODE_BGC;
    stUsrPic.unUsrPic.stUsrPicBg.u32BgColor = 0xee2200;
    s32Ret = HI_MPI_VI_SetUserPic(ViPipe, &stUsrPic);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("HI_MPI_VI_SetUserPic failed with %#x!\n", s32Ret);
        goto EXIT4;
    }

    printf("Press Enter key to Disable UsePic!\n");
    getchar();

    s32Ret = HI_MPI_VI_DisableUserPic(ViPipe);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("HI_MPI_VI_DisableUserPic failed with %#x!\n", s32Ret);
        goto EXIT4;
    }

    PAUSE();

EXIT4:
    SAMPLE_COMM_VI_Release_UserPic(&stViFrameInfo);
EXIT3:
    SAMPLE_COMM_VI_UnBind_VO(ViPipe, ViChn, stVoConfig.VoDev, VoChn);
EXIT2:
    SAMPLE_COMM_VO_StopVO(&stVoConfig);
EXIT1:
    SAMPLE_COMM_VI_StopVi(&stViConfig);
EXIT:
    SAMPLE_COMM_SYS_Exit();
    return s32Ret;
}

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */
