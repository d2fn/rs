/*******************************************************************************************
*
*   Rs v1.0.0 - Tool Description
*
*   MODULE USAGE:
*       #define GUI_RS_IMPLEMENTATION
*       #include "gui_rs.h"
*
*       INIT: GuiRsState state = InitGuiRs();
*       DRAW: GuiRs(&state);
*
*   LICENSE: Propietary License
*
*   Copyright (c) 2022 raylib technologies. All Rights Reserved.
*
*   Unauthorized copying of this file, via any medium is strictly prohibited
*   This project is proprietary and confidential unless the owner allows
*   usage in any other form by expresely written permission.
*
**********************************************************************************************/

#include "raylib.h"

// WARNING: raygui implementation is expected to be defined before including this header
#undef RAYGUI_IMPLEMENTATION
#include "raygui.h"

#include <string.h>     // Required for: strcpy()

#ifndef GUI_RS_H
#define GUI_RS_H

typedef struct {
    float GravitySliderValue;
    float DragSliderValue;
    int ColorMode;
    int ColorType;
    float ParticleScaleSliderValue;
    float ReplicationRateSliderValue;

    Rectangle layoutRecs[14];

    // Custom state variables (depend on development software)
    // NOTE: This variables should be added manually if required

} GuiRsState;

#ifdef __cplusplus
extern "C" {            // Prevents name mangling of functions
#endif

//----------------------------------------------------------------------------------
// Defines and Macros
//----------------------------------------------------------------------------------
//...

//----------------------------------------------------------------------------------
// Types and Structures Definition
//----------------------------------------------------------------------------------
// ...

//----------------------------------------------------------------------------------
// Module Functions Declaration
//----------------------------------------------------------------------------------
GuiRsState InitGuiRs(void);
int GuiRs(GuiRsState *state);


#ifdef __cplusplus
}
#endif

#endif // GUI_RS_H

/***********************************************************************************
*
*   GUI_RS IMPLEMENTATION
*
************************************************************************************/
#if defined(GUI_RS_IMPLEMENTATION)

#include "raygui.h"

//----------------------------------------------------------------------------------
// Global Variables Definition
//----------------------------------------------------------------------------------
//...

//----------------------------------------------------------------------------------
// Internal Module Functions Definition
//----------------------------------------------------------------------------------
//...

//----------------------------------------------------------------------------------
// Module Functions Definition
//----------------------------------------------------------------------------------
GuiRsState InitGuiRs(void)
{
    GuiRsState state = { 0 };

    state.GravitySliderValue = 1e-4f;
    state.DragSliderValue = 0.01f;
    state.ColorMode = 0;
    state.ColorType = 0;
    state.ParticleScaleSliderValue = 0.03f;
    state.ReplicationRateSliderValue = 0.0f;

    state.layoutRecs[0] = (Rectangle){ 24, 184, 360, 192 };
    state.layoutRecs[1] = (Rectangle){ 96, 208, 128, 24 };
    state.layoutRecs[2] = (Rectangle){ 96, 240, 128, 24 };
    state.layoutRecs[3] = (Rectangle){ 24, 24, 360, 136 };
    state.layoutRecs[4] = (Rectangle){ 40, 40, 160, 24 };
    state.layoutRecs[5] = (Rectangle){ 40, 80, 64, 24 };
    state.layoutRecs[6] = (Rectangle){ 136, 120, 136, 24 };
    state.layoutRecs[7] = (Rectangle){ 232, 208, 136, 24 };
    state.layoutRecs[8] = (Rectangle){ 232, 240, 136, 24 };
    state.layoutRecs[9] = (Rectangle){ 40, 272, 328, 16 };
    state.layoutRecs[10] = (Rectangle){ 160, 296, 104, 24 };
    state.layoutRecs[11] = (Rectangle){ 272, 296, 96, 24 };
    state.layoutRecs[12] = (Rectangle){ 280, 120, 88, 24 };
    state.layoutRecs[13] = (Rectangle){ 40, 336, 328, 24 };

    // Custom variables initialization

    return state;
}

int GuiRs(GuiRsState *state)
{
    GuiGroupBox(state->layoutRecs[0], "Physics");
    GuiSliderBar(state->layoutRecs[1], "Gravity", NULL, &state->GravitySliderValue, 1e-5, 1e-3);
    GuiSliderBar(state->layoutRecs[2], "Drag", NULL, &state->DragSliderValue, 0.0, 0.2);
    GuiGroupBox(state->layoutRecs[3], "Theme");
    GuiToggleGroup(state->layoutRecs[4], "Dark;Light", &state->ColorMode);
    GuiToggleGroup(state->layoutRecs[5], "B&W;Grayscale;Color;Halftone;Field", &state->ColorType);
    GuiSliderBar(state->layoutRecs[6], "PartcleScale", NULL, &state->ParticleScaleSliderValue, 0.0001, 0.1);
    GuiLabel(state->layoutRecs[7], "GravityLabel");
    GuiLabel(state->layoutRecs[8], "DragLabel");
    GuiLine(state->layoutRecs[9], NULL);
    GuiSliderBar(state->layoutRecs[10], "Replication Rate", NULL, &state->ReplicationRateSliderValue, 0, 100);
    GuiLabel(state->layoutRecs[11], "ReplicationRateLabel");
    GuiLabel(state->layoutRecs[12], "ParticleScaleLabel");
    if (GuiButton(state->layoutRecs[13], "Clear Particles")) {
        return 1;
    }
    return 0;
}

#endif // GUI_RS_IMPLEMENTATION
