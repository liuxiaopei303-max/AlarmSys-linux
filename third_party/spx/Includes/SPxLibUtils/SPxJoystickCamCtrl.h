/*********************************************************************
 *
 * (c) Copyright 2015 - 2017, Cambridge Pixel Ltd.
 *
 * File: $RCSfile: SPxJoystickCamCtrl.h,v $
 * ID: $Id: SPxJoystickCamCtrl.h,v 1.4 2017/09/11 14:41:32 rew Exp $
 *
 * Purpose:
 *   Joystick class for controlling a camera.
 *
 * Revision Control:
 *  11/09/17 v1.4    SP 	Match base class change to SetOnScreenControl().
 *
 * Previous Changes:
 *  29/08/17 1.3    AGC	Match base class change to SetOnScreenControl().
 *  19/10/16 1.2    AGC	Support presets.
 *			Support on-screen joystick interaction.
 *  29/06/15 1.1    AGC	Initial version.
 *
 **********************************************************************/
#ifndef SPX_JOYSTICK_CAM_CTRL_H_
#define SPX_JOYSTICK_CAM_CTRL_H_

/* Library headers. */
#include "SPxLibUtils/SPxAutoPtr.h"
#include "SPxLibUtils/SPxJoystickInterface.h"

/* Forward declarations. */
class SPxCamera;
#ifdef _WIN32
class SPxJoystickWin;
#endif

/* SPxJoystickInterface dialog */
class SPxJoystickCamCtrl : public SPxJoystickInterface
{
public:
    SPxJoystickCamCtrl(void);
    virtual ~SPxJoystickCamCtrl(void);

    SPxErrorCode SetCamera(SPxCamera *camera);

#ifdef _WIN32
    virtual SPxErrorCode SetOnScreenControl(SPxJoystickWin *ctrl,
					    int ctrlWhenVisible=TRUE,
					    int ctrlAxesEnables=TRUE);
#endif

    SPxErrorCode SetSavePresets(int savePresets);
    SPxErrorCode SetPreset(SPxJoystickInterface::Button_t button,
			   UINT8 preset);
    SPxErrorCode GetPreset(SPxJoystickInterface::Button_t button,
			   UINT8 *preset) const;
    SPxErrorCode ClearPreset(SPxJoystickInterface::Button_t button);
    SPxErrorCode SetPresetController(SPxJoystickInterface::Button_t button,
				     int controller);
    SPxErrorCode GetPresetController(SPxJoystickInterface::Button_t button,
				     int *controller) const;

    virtual int SetParameter(char *name, char *value);
    virtual int GetParameter(char *name, char *valueBuf, int bufLen);
    SPxErrorCode SetConfigFromState(void);

private:
    struct impl;
    SPxAutoPtr<impl> m_p;

    static void *threadFnWrapper(SPxThread *thread);
    void *threadFn(SPxThread *thread);
    void updateCamera(void);
    static void buttonReleasedWrapper(void *userArg, SPxJoystickInterface::Button_t button);
    void buttonReleased(SPxJoystickInterface::Button_t button);
    static int deleteCameraWrapper(void *invokingObject, void *userObject, void *arg);
    int deleteCamera(SPxCamera *camera);
#ifdef _WIN32
    static void osBtnHandler(unsigned int index, void *userArg);
#endif
};

#endif /* SPX_JOYSTICK_CAM_CTRL_H_ */
/*********************************************************************
 *
 * End of file
 *
 **********************************************************************/
