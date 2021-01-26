#include "homerFace.h"
#include "config.h"
#include "gui.h"

LV_IMG_DECLARE(homerFace);
LV_IMG_DECLARE(hourHand);
LV_IMG_DECLARE(minHand);
LV_IMG_DECLARE(secHand);
LV_IMG_DECLARE(eye);

typedef struct
{
  lv_task_t *timeTask;
  lv_obj_t *hourHand;
  lv_obj_t *minHand;
  lv_obj_t *secHand;
  lv_obj_t *hourShadow;
  lv_obj_t *minShadow;
  lv_obj_t *secShadow;
  lv_obj_t *leftEye;
  lv_obj_t *rightEye;
} HomerFaceData_t;

static int updateHomerFace (int idx)
{
  time_t now;
  struct tm info;
  HomerFaceData_t *hfd = (HomerFaceData_t *)(tileDesc[idx].data);
  static int lastMin = 61;
  
  log_d ("idx=%d", idx);

  time (&now);
  localtime_r (&now, &info);

  int sec  = 60 * info.tm_sec;

  lv_img_set_angle (hfd->leftEye, sec);
  lv_img_set_angle (hfd->rightEye, sec);

  if (info.tm_min != lastMin)
  {
    lastMin = info.tm_min;
    
    int hour = (300 * (info.tm_hour % 12)) + (5 * lastMin);
    int min  = 60 * lastMin;

    lv_img_set_angle (hfd->minShadow, min);
    lv_img_set_angle (hfd->minHand, min);

    lv_img_set_angle (hfd->hourShadow, hour);
    lv_img_set_angle (hfd->hourHand, hour);
  }

  lv_img_set_angle (hfd->secShadow, sec);
  lv_img_set_angle (hfd->secHand, sec);
}

static int onEntryHomerFace (int idx)
{
  HomerFaceData_t *hfd = (HomerFaceData_t *)(tileDesc[idx].data);

  log_d ("idx=%d", idx);

  screenTimeout = defaultScreenTimeout = DEFAULT_SCREEN_TIMEOUT * 5;
  defaultCpuFrequency = CPU_FREQ_MAX;
  setCpuFrequencyMhz (defaultCpuFrequency);

  if (hfd->timeTask == nullptr)
  {
    hfd->timeTask = lv_task_create (lv_update_task, 1000, LV_TASK_PRIO_LOWEST, (void *)idx);
  }
}

static int onExitHomerFace (int idx)
{
  HomerFaceData_t *hfd = (HomerFaceData_t *)(tileDesc[idx].data);

  log_d ("idx=%d", idx);

  if (hfd->timeTask != nullptr)
  {
    lv_task_del (hfd->timeTask);
    hfd->timeTask = nullptr;
  }
  
  screenTimeout = defaultScreenTimeout = DEFAULT_SCREEN_TIMEOUT;
  defaultCpuFrequency = CPU_FREQ_NORM;
}

int createHomerFace (int idx)
{
  lv_obj_t *parentObj = tileDesc[idx].tile;
  lv_obj_t *canvas = lv_canvas_create (parentObj, NULL);
  lv_color_t *cbuf = (lv_color_t *)ps_malloc(LV_CANVAS_BUF_SIZE_TRUE_COLOR_ALPHA(240, 240) * sizeof (lv_color_t));

  memcpy (cbuf, homerFace.data, homerFace.data_size);
  
  lv_canvas_set_buffer (canvas, cbuf, 240, 240, LV_IMG_CF_TRUE_COLOR_ALPHA);
  
  lv_draw_line_dsc_t ticks;
  lv_draw_line_dsc_init (&ticks);
  ticks.opa = LV_OPA_100;
  ticks.color = LV_COLOR_WHITE;

  lv_point_t line[2];
  
  for (int i = 0; i < 60; i++)
  {
    int bot;
    float sx = cos(((i * 6) - 90) * 0.0174532925);
    float sy = sin(((i * 6) - 90) * 0.0174532925);
    
    line[0].x = sx * 117 + 120;
    line[0].y = sy * 117 + 120;

    if ((i % 15) == 0)
    {
      bot = 97;
      ticks.width = 6;
    }
    else if ((i % 5) == 0)
    {
      bot = 107;
      ticks.width = 4;
    }
    else
    {
      bot = 112;
      ticks.width = 2;
    }
    
    line[1].x = sx * bot + 120;
    line[1].y = sy * bot + 120;
    
    lv_canvas_draw_line (canvas, line, 2, &ticks);
  }
  
  lv_obj_align (canvas, parentObj, LV_ALIGN_CENTER, 0, 0);

  lv_obj_set_event_cb (parentObj, watchFaceEvent_cb);

  tileDesc[idx].onEntry  = onEntryHomerFace;
  tileDesc[idx].onExit   = onExitHomerFace;
  tileDesc[idx].onUpdate = updateHomerFace;

  tileDesc[idx].data = malloc (sizeof (HomerFaceData_t));

  HomerFaceData_t *hfd = (HomerFaceData_t *)(tileDesc[idx].data);

  hfd->leftEye = lv_img_create (parentObj, NULL);
  hfd->rightEye = lv_img_create (parentObj, NULL);

  hfd->minShadow  = lv_img_create (parentObj, NULL);
  hfd->minHand  = lv_img_create (parentObj, NULL);

  hfd->hourShadow = lv_img_create (parentObj, NULL);
  hfd->hourHand = lv_img_create (parentObj, NULL);

  hfd->secShadow  = lv_img_create (parentObj, NULL);
  hfd->secHand  = lv_img_create (parentObj, NULL);

  lv_img_set_src (hfd->leftEye, &eye);
  lv_img_set_src (hfd->rightEye, &eye);

  lv_img_set_src (hfd->hourShadow, &hourHand); lv_img_set_pivot (hfd->hourShadow, 7,  77);
  lv_img_set_src (hfd->minShadow,  &minHand);  lv_img_set_pivot (hfd->minShadow,  7, 105);
  lv_img_set_src (hfd->secShadow,  &secHand);  lv_img_set_pivot (hfd->secShadow, 22, 90);

  lv_img_set_src (hfd->hourHand, &hourHand); lv_img_set_pivot (hfd->hourHand, 7,  77);
  lv_img_set_src (hfd->minHand,  &minHand);  lv_img_set_pivot (hfd->minHand,  7, 105);
  lv_img_set_src (hfd->secHand,  &secHand);  lv_img_set_pivot (hfd->secHand, 22, 90);

  lv_obj_align (hfd->leftEye, parentObj, LV_ALIGN_CENTER, -30, -28);
  lv_obj_align (hfd->rightEye, parentObj, LV_ALIGN_CENTER, 30, -28);

  lv_obj_align (hfd->hourShadow, parentObj, LV_ALIGN_CENTER, 0, -32);
  lv_obj_align (hfd->minShadow,  parentObj, LV_ALIGN_CENTER, 0, -41);
  lv_obj_align (hfd->secShadow,  parentObj, LV_ALIGN_CENTER, 3, -15);

  lv_obj_align (hfd->hourHand, parentObj, LV_ALIGN_CENTER, 1, -40);
  lv_obj_align (hfd->minHand,  parentObj, LV_ALIGN_CENTER, 1, -49);
  lv_obj_align (hfd->secHand,  parentObj, LV_ALIGN_CENTER, 4, -26);

  static lv_style_t shadowStyle;

  lv_style_init (&shadowStyle);
  lv_style_set_radius (&shadowStyle, LV_OBJ_PART_MAIN, 0);
  lv_style_set_image_recolor (&shadowStyle, LV_STATE_DEFAULT, LV_COLOR_BLACK);
  lv_style_set_image_recolor_opa (&shadowStyle, LV_STATE_DEFAULT, LV_OPA_100);
  lv_style_set_image_opa (&shadowStyle, LV_STATE_DEFAULT, 63);  // LV_OPA_25ish!
  lv_style_set_border_width (&shadowStyle, LV_OBJ_PART_MAIN, 0);

  lv_obj_add_style (hfd->hourShadow, LV_IMG_PART_MAIN, &shadowStyle);
  lv_obj_add_style (hfd->minShadow, LV_IMG_PART_MAIN, &shadowStyle);
  lv_obj_add_style (hfd->secShadow, LV_IMG_PART_MAIN, &shadowStyle);

  hfd->timeTask = nullptr;

  updateHomerFace (idx);
}
