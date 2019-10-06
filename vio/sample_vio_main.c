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

#include "hi_common.h"
#include "sample_vio.h"

/******************************************************************************
* function : show usage
******************************************************************************/
HI_VOID SAMPLE_VIO_VoInterface_Usage(HI_VOID)
{
    printf("intf:\n");
    printf("\t 0) vo HDMI output.\n");
    printf("\t 1) vo BT1120 output, default.\n");
}

void SAMPLE_VIO_Usage(char *sPrgNm)
{
    printf("Usage : %s <index> <intf>\n", sPrgNm);
    printf("index:\n");
    printf("\t 0)VI(Online) - VPSS(Online) - VO.\n");
    printf("\t 1)VI(Offline)- VPSS(Offline) - VO. LDC+DIS+SPREAD.\n");
    printf("\t 2)VI(Offline)- VPSS(Online) - VO.  Double pipe.\n");
    printf("\t 3)VI(Online)- VPSS(Offline) - VO.  Double chn.\n");
    printf("\t 4)Resolute Ratio Switch.\n");
    printf("\t 5)GDC - VPSS LowDelay.\n");
    printf("\t 6)Double WDR Pipe.\n");
    printf("\t 7)FPN Calibrate & Correction.\n");
    printf("\t 8)WDR Switch.\n");
    printf("\t 9)90/180/270 Rotate.\n");
    printf("\t 10)Mipi Demux Yuv.\n");
    printf("\t 11)UserPic.\n");

    printf("intf:\n");
    printf("\t 0) vo HDMI output, default.\n");
    printf("\t 1) vo BT1120 output.\n");
    return;
}

/******************************************************************************
* function    : main()
* Description : main
******************************************************************************/
#ifdef __HuaweiLite__
int app_main(int argc, char *argv[])
#else
int main(int argc, char *argv[])
#endif
{
    HI_S32 s32Ret = HI_FAILURE;
    HI_S32 s32Index;
    HI_U32 u32VoIntfType = 0;

    if (argc < 2)
    {
        SAMPLE_VIO_Usage(argv[0]);
        return HI_FAILURE;
    }

#ifdef __HuaweiLite__
#else
    signal(SIGINT, SAMPLE_VIO_HandleSig);
    signal(SIGTERM, SAMPLE_VIO_HandleSig);
#endif

    if ((argc > 2) && (1 == atoi(argv[2])))
    {
        u32VoIntfType = 1;
    }

    SAMPLE_VIO_MsgInit();

    s32Index = atoi(argv[1]);
    switch (s32Index)
    {
        case 0:
            s32Ret = SAMPLE_VIO_ViOnlineVpssOnlineRoute(u32VoIntfType);
            break;

        case 1:
            s32Ret = SAMPLE_VIO_ViOnlineVpssOfflineRoute(u32VoIntfType);
            break;

        case 2:
            s32Ret = SAMPLE_VIO_ViDoublePipeRoute(u32VoIntfType);
            break;

        case 3:
            s32Ret = SAMPLE_VIO_ViDoubleChnRoute(u32VoIntfType);
            break;

        case 4:
            s32Ret = SAMPLE_VIO_ViResoSwitch(u32VoIntfType);
            break;

        case 5:
            s32Ret = SAMPLE_VIO_ViVpssLowDelay(u32VoIntfType);
            break;

        case 6:
            s32Ret = SAMPLE_VIO_ViDoubleWdrPipe(u32VoIntfType);
            break;

        case 7:
            s32Ret = SAMPLE_VIO_ViFPN(u32VoIntfType);
            break;

        case 8:
            s32Ret = SAMPLE_VIO_ViWdrSwitch(u32VoIntfType);
            break;

        case 9:
            s32Ret = SAMPLE_VIO_ViRotate(u32VoIntfType);
            break;

        case 10:
            s32Ret = SAMPLE_VIO_ViDeMuxYuv(u32VoIntfType);
            break;

        case 11:
            s32Ret = SAMPLE_VIO_ViSetUsrPic(u32VoIntfType);
            break;

        default:
            SAMPLE_PRT("the index %d is invaild!\n",s32Index);
            SAMPLE_VIO_Usage(argv[0]);
            SAMPLE_VIO_MsgExit();
            return HI_FAILURE;
    }

    if (HI_SUCCESS == s32Ret)
    {
        SAMPLE_PRT("sample_vio exit success!\n");
    }
    else
    {
        SAMPLE_PRT("sample_vio exit abnormally!\n");
    }

    SAMPLE_VIO_MsgExit();

    return s32Ret;
}

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */
