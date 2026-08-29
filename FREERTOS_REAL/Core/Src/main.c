/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "ssd1306.h"
#include "ssd1306_fonts.h"
#include "stdio.h"
#include "string.h"
#include "DHT.h"
#define LIGHT_RELAY_PIN      GPIO_PIN_13
#define LIGHT_RELAY_PORT     GPIOB

#define PUMP_RELAY_PIN       GPIO_PIN_14
#define PUMP_RELAY_PORT      GPIOB

#define SOIL_PUMP_ON_THRESH  60
#define LIGHT_LOW_THRESH     40

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef enum {
  BTN_EVT_OK = 0,
  BTN_EVT_BACK,
  BTN_EVT_UP,
  BTN_EVT_DOWN
} ButtonEvent_t;

typedef enum {
  SCREEN_MENU = 0,
  SCREEN_PLANT_LIST,
  SCREEN_PLANT_INFO,
  SCREEN_PLANT_CONFIRM,
  SCREEN_ACTIVE_HOME,
  SCREEN_ACTIVE_INFO,
  SCREEN_HISTORY_LIST,
  SCREEN_HISTORY_DETAIL
} ScreenState_t;

typedef struct {
  const char* name;
  uint8_t tempMin;
  uint8_t tempMax;
  uint8_t soilMin;
  uint8_t soilMax;
  uint8_t humidMin;
  uint8_t humidMax;
  uint8_t lightMin;
  uint8_t lightMax;
  uint8_t waterTimesPerDay;
} PlantData;

typedef struct {
  int soil;
  int humid;
  int temp;
  int lightPercent;
  float lightHours;
  int minutesToWater;
  int dayRemainSeconds;
  float careHours;
  uint8_t pumpState;
  uint8_t lightState;
} SensorData_t;

typedef struct {
  uint8_t used;
  uint8_t plantId;
  uint16_t reserved;
} HistoryItem_t;

#define HISTORY_MAX 5

typedef struct {
  uint32_t magic;
  uint8_t hasActivePlant;
  uint8_t activePlantId;
  uint8_t historyCount;
  uint8_t historyWriteIndex;

  int minutesToWater;
  float lightHours;

  HistoryItem_t history[HISTORY_MAX];
} PersistData_t;


/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define BTN_OK_PIN        GPIO_PIN_2
#define BTN_OK_PORT       GPIOA

#define BTN_BACK_PIN      GPIO_PIN_1
#define BTN_BACK_PORT     GPIOA

#define BTN_UP_PIN        GPIO_PIN_3
#define BTN_UP_PORT       GPIOA

#define BTN_DOWN_PIN      GPIO_PIN_0
#define BTN_DOWN_PORT     GPIOA

#define DEBOUNCE_MS               20U
#define SENSOR_PERIOD_MS          1000U
#define ACTIVE_HOME_REFRESH_MS    1000U

#define EVT_REDRAW                (1U << 0)

#define PLANT_COUNT               3

#define FLASH_STORE_ADDR          0x0800FC00U
#define PERSIST_MAGIC             0x504C414EU

DHT_Name DHT1;
uint8_t dhtOk = 0;
float gDhtTempCache = 0.0f;
float gDhtHumidCache = 0.0f;
uint8_t gDhtHasValidData = 0;


uint16_t adcSoilRaw = 0;   // PA4
uint16_t adcLightRaw = 0;  // PA5
uint16_t adcAuxRaw = 0;    // PA6

float gTimeScale = 1.0f;   // he so toc do thoi gian

ScreenState_t currentScreen = SCREEN_MENU;

uint8_t menuIndex = 0;
uint8_t plantIndex = 0;
uint8_t previewPlantId = 0;
uint8_t activePlantId = 0;
uint8_t confirmIndex = 0;
uint8_t historyIndex = 0;

uint8_t infoScroll = 0;
uint8_t activeInfoScroll = 0;
uint8_t homeScroll = 0;
uint8_t hasActivePlant = 0;

SensorData_t gSensorData = {
  .soil = 0,
  .humid = 40,
  .temp = 37,
  .lightPercent = 0,
  .lightHours = 0.0f,
  .minutesToWater = 0,
  .dayRemainSeconds = 24 * 60 * 60,
  .careHours = 0.0f,
  .pumpState = 0,
  .lightState = 0
};
PersistData_t gPersist = {0};

PlantData plants[PLANT_COUNT] = {
  {"Dua leo", 25, 32, 65, 80, 60, 85, 6, 8, 2},
  {"Xa lach", 18, 25, 60, 70, 60, 80, 4, 6, 2},
  {"Nam",     20, 28, 70, 85, 80, 95, 1, 2, 3}
};

//debug
uint32_t dhtValidCount = 0;


/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;

I2C_HandleTypeDef hi2c1;

TIM_HandleTypeDef htim3;

UART_HandleTypeDef huart1;

/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* USER CODE BEGIN PV */

osThreadId_t inputTaskHandle;
osThreadId_t sensorTaskHandle;
osThreadId_t logicTaskHandle;
osThreadId_t displayTaskHandle;
osThreadId_t dhtTaskHandle;

const osThreadAttr_t inputTask_attributes = {
  .name = "inputTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityAboveNormal,
};

const osThreadAttr_t sensorTask_attributes = {
  .name = "sensorTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityLow,
};

const osThreadAttr_t logicTask_attributes = {
  .name = "logicTask",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};

const osThreadAttr_t displayTask_attributes = {
  .name = "displayTask",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityBelowNormal,
};

const osThreadAttr_t dhtTask_attributes = {
  .name = "dhtTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityLow1,
};


osMessageQueueId_t buttonQueueHandle;
osMutexId_t appMutexHandle;
osMutexId_t oledMutexHandle;
osEventFlagsId_t uiEventHandle;

const osMutexAttr_t appMutex_attributes = {
  .name = "appMutex"
};

const osMutexAttr_t oledMutex_attributes = {
  .name = "oledMutex"
};



/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
static void MX_ADC1_Init(void);
static void MX_TIM3_Init(void);
static void MX_USART1_UART_Init(void);
void StartDefaultTask(void *argument);

/* USER CODE BEGIN PFP */
void StartInputTask(void *argument);
void StartSensorTask(void *argument);
void StartLogicTask(void *argument);
void StartDisplayTask(void *argument);
void StartDHTTask(void *argument);

static uint8_t ReadButtonPressed(GPIO_TypeDef *port, uint16_t pin);
static void ProcessButton(GPIO_TypeDef *port, uint16_t pin, ButtonEvent_t evt,
                          uint8_t *sta_now, uint8_t *sta_pre, uint8_t *sta_final,
                          uint8_t *stable_last, uint8_t *flag_press, uint32_t *time_flag);

static void RequestRedraw(void);
static void UpdateSensorData(void);
static void RenderScreen(void);
static void Persist_Load(void);
static void Persist_Save(void);
static void Persist_ApplyToRuntime(void);
static void Persist_UpdateFromRuntime(void);
static void History_Add(uint8_t plantId);

static void HandleMenuInput(ButtonEvent_t evt);
static void HandlePlantListInput(ButtonEvent_t evt);
static void HandlePlantInfoInput(ButtonEvent_t evt);
static void HandlePlantConfirmInput(ButtonEvent_t evt);
static void HandleActiveHomeInput(ButtonEvent_t evt);
static void HandleActiveInfoInput(ButtonEvent_t evt);
static void HandleHistoryListInput(ButtonEvent_t evt);
static void HandleHistoryDetailInput(ButtonEvent_t evt);
void Error_Blink(uint8_t count);
static void DrawHistoryDetailScreen(void);
static void UpdateSensorData(void);
static uint16_t Read_ADC_Channel(uint32_t channel);
static int Map_ADC_To_Percent(uint16_t value, uint16_t minAdc, uint16_t maxAdc, uint8_t invert);
static uint8_t DHT_Read_Safe(float *t, float *h);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
static uint16_t Read_ADC_Channel(uint32_t channel)
{
  ADC_ChannelConfTypeDef sConfig = {0};

  sConfig.Channel = channel;
  sConfig.Rank = ADC_REGULAR_RANK_1;
  sConfig.SamplingTime = ADC_SAMPLETIME_55CYCLES_5;

  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK) {
    Error_Blink(12);
  }

  if (HAL_ADC_Start(&hadc1) != HAL_OK) {
    Error_Blink(13);
  }

  if (HAL_ADC_PollForConversion(&hadc1, 10) != HAL_OK) {
    HAL_ADC_Stop(&hadc1);
    Error_Blink(14);
  }

  uint16_t value = HAL_ADC_GetValue(&hadc1);
  HAL_ADC_Stop(&hadc1);

  return value;
}


static float Map_ADC_To_TimeScale(uint16_t adc)
{
  float scale;


  scale = 1.0f + ((float)adc / 4095.0f) * 999.0f;

  return scale;
}
static int Map_ADC_To_Percent(uint16_t value, uint16_t minAdc, uint16_t maxAdc, uint8_t invert)
{
  int percent;

  if (maxAdc <= minAdc) return 0;

  if (value < minAdc) value = minAdc;
  if (value > maxAdc) value = maxAdc;

  percent = ((int)(value - minAdc) * 100) / (int)(maxAdc - minAdc);

  if (invert) {
    percent = 100 - percent;
  }

  if (percent < 0) percent = 0;
  if (percent > 100) percent = 100;

  return percent;
}


static void Persist_Load(void)
{
  const PersistData_t *p = (const PersistData_t *)FLASH_STORE_ADDR;

  if (p->magic == PERSIST_MAGIC) {
    memcpy(&gPersist, p, sizeof(PersistData_t));
  } else {
    memset(&gPersist, 0, sizeof(PersistData_t));
    gPersist.magic = PERSIST_MAGIC;
  }
}

static void Persist_UpdateFromRuntime(void)
{
  gPersist.magic = PERSIST_MAGIC;
  gPersist.hasActivePlant = hasActivePlant;
  gPersist.activePlantId = activePlantId;
  gPersist.minutesToWater = gSensorData.minutesToWater;
  gPersist.lightHours = gSensorData.lightHours;
}

static void Persist_ApplyToRuntime(void)
{
  hasActivePlant = gPersist.hasActivePlant;
  activePlantId = gPersist.activePlantId;
  gSensorData.minutesToWater = gPersist.minutesToWater;
  gSensorData.lightHours = gPersist.lightHours;
}

static void Persist_Save(void)
{
  HAL_FLASH_Unlock();

  FLASH_EraseInitTypeDef eraseInit;
  uint32_t pageError = 0;

  eraseInit.TypeErase = FLASH_TYPEERASE_PAGES;
  eraseInit.PageAddress = FLASH_STORE_ADDR;
  eraseInit.NbPages = 1;

  if (HAL_FLASHEx_Erase(&eraseInit, &pageError) != HAL_OK) {
    HAL_FLASH_Lock();
    Error_Blink(10);
  }

  uint32_t *src = (uint32_t *)&gPersist;
  uint32_t addr = FLASH_STORE_ADDR;
  uint32_t words = (sizeof(PersistData_t) + 3U) / 4U;

  for (uint32_t i = 0; i < words; i++) {
    if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, addr, src[i]) != HAL_OK) {
      HAL_FLASH_Lock();
      Error_Blink(11);
    }
    addr += 4;
  }

  HAL_FLASH_Lock();
}

static void History_Add(uint8_t plantId)
{
  uint8_t idx = gPersist.historyWriteIndex;

  gPersist.history[idx].used = 1;
  gPersist.history[idx].plantId = plantId;

  idx++;
  if (idx >= HISTORY_MAX) idx = 0;

  gPersist.historyWriteIndex = idx;

  if (gPersist.historyCount < HISTORY_MAX) {
    gPersist.historyCount++;
  }
}

static uint8_t ReadButtonPressed(GPIO_TypeDef *port, uint16_t pin)
{
  return (HAL_GPIO_ReadPin(port, pin) == GPIO_PIN_RESET) ? 1U : 0U;
}

static void ProcessButton(GPIO_TypeDef *port, uint16_t pin, ButtonEvent_t evt,
                          uint8_t *sta_now, uint8_t *sta_pre, uint8_t *sta_final,
                          uint8_t *stable_last, uint8_t *flag_press, uint32_t *time_flag)
{
  uint8_t button_read = ReadButtonPressed(port, pin);
  *sta_now = button_read;

  if (*sta_pre != *sta_now) {
    *sta_pre = *sta_now;
    *flag_press = 1;
    *time_flag = osKernelGetTickCount();
  }

  if (*flag_press && ((osKernelGetTickCount() - *time_flag) > DEBOUNCE_MS)) {
    *sta_final = *sta_pre;
    *flag_press = 0;
  }

  if ((*stable_last == 0U) && (*sta_final == 1U)) {
    osMessageQueuePut(buttonQueueHandle, &evt, 0, 0);
  }

  *stable_last = *sta_final;
}

//tạm fake
static void UpdateSensorData(void)
{
  osMutexAcquire(appMutexHandle, osWaitForever);

  adcSoilRaw  = Read_ADC_Channel(ADC_CHANNEL_4);
  adcLightRaw = Read_ADC_Channel(ADC_CHANNEL_5);
  adcAuxRaw   = Read_ADC_Channel(ADC_CHANNEL_6);

  gTimeScale = Map_ADC_To_TimeScale(adcAuxRaw);

  gSensorData.soil = Map_ADC_To_Percent(adcSoilRaw, 1200, 3200, 1);
  gSensorData.lightPercent = Map_ADC_To_Percent(adcLightRaw, 0, 4095, 0);

  if (hasActivePlant) {
    float dt = 1.0f * gTimeScale;
    int deltaSec = (int)(dt + 0.5f);
    PlantData *p = &plants[activePlantId];

    if (deltaSec < 1) deltaSec = 1;

    /* tong thoi gian cham cay, cong don lien tuc */
    gSensorData.careHours += dt / 3600.0f;

    /* thoi gian con lai trong ngay */
    gSensorData.dayRemainSeconds -= deltaSec;

    /* neu sang ngay moi -> reset gio sang trong ngay moi */
    while (gSensorData.dayRemainSeconds <= 0) {
      gSensorData.dayRemainSeconds += 24 * 60 * 60;
      gSensorData.lightHours = 0.0f;
    }

    /* dieu khien bom theo do am dat */
    if (gSensorData.soil < p->soilMin) {
      gSensorData.pumpState = 1;
      HAL_GPIO_WritePin(PUMP_RELAY_PORT, PUMP_RELAY_PIN, GPIO_PIN_SET);
    } else {
      gSensorData.pumpState = 0;
      HAL_GPIO_WritePin(PUMP_RELAY_PORT, PUMP_RELAY_PIN, GPIO_PIN_RESET);
    }

    /* dieu khien den:
       - duoi 40% thi coi la thieu sang
       - neu chua du so gio sang toi thieu trong ngay thi bat den */
    if ((gSensorData.lightPercent < LIGHT_LOW_THRESH) &&
        (gSensorData.lightHours < (float)p->lightMin)) {
      gSensorData.lightState = 1;
      HAL_GPIO_WritePin(LIGHT_RELAY_PORT, LIGHT_RELAY_PIN, GPIO_PIN_SET);

      /* khi den dang bat thi van tinh la dang duoc chieu sang */
      gSensorData.lightHours += dt / 3600.0f;
    } else {
      gSensorData.lightState = 0;
      HAL_GPIO_WritePin(LIGHT_RELAY_PORT, LIGHT_RELAY_PIN, GPIO_PIN_RESET);

      /* neu khong bat den ma anh sang moi truong du lon, van cong gio sang */
      if (gSensorData.lightPercent >= LIGHT_LOW_THRESH) {
        gSensorData.lightHours += dt / 3600.0f;
      }
    }

    /* gioi han de khong vuot qua muc toi da cua cay trong 1 ngay */
    if (gSensorData.lightHours > (float)p->lightMax) {
      gSensorData.lightHours = (float)p->lightMax;
    }

    if (gSensorData.minutesToWater > 0) {
      gSensorData.minutesToWater -= (int)(dt / 60.0f);
      if (gSensorData.minutesToWater < 0) {
        gSensorData.minutesToWater = 0;
      }
    }

  } else {
    gSensorData.lightHours = 0.0f;
    gSensorData.dayRemainSeconds = 24 * 60 * 60;
    gSensorData.careHours = 0.0f;
    gSensorData.pumpState = 0;
    gSensorData.lightState = 0;

    HAL_GPIO_WritePin(PUMP_RELAY_PORT, PUMP_RELAY_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LIGHT_RELAY_PORT, LIGHT_RELAY_PIN, GPIO_PIN_RESET);
  }

  osMutexRelease(appMutexHandle);
}



static uint8_t DHT_Read_Safe(float *t, float *h)
{
  uint8_t ok;

  __disable_irq();
  ok = DHT_ReadTempHum(&DHT1);
  if (ok) {
    *t = DHT1.Temp;
    *h = DHT1.Humi;
  }
  __enable_irq();

  return ok;
}



static void RequestRedraw(void)
{
  osEventFlagsSet(uiEventHandle, EVT_REDRAW);
}

static void HandleMenuInput(ButtonEvent_t evt)
{
  if (evt == BTN_EVT_UP && menuIndex > 0) menuIndex--;
  if (evt == BTN_EVT_DOWN && menuIndex < 1) menuIndex++;

  if (evt == BTN_EVT_OK) {
    if (menuIndex == 0) {
      currentScreen = SCREEN_PLANT_LIST;
      plantIndex = 0;
    } else {
      currentScreen = SCREEN_HISTORY_LIST;
      historyIndex = 0;
    }
  }

  if (evt == BTN_EVT_BACK && hasActivePlant) {
    currentScreen = SCREEN_ACTIVE_HOME;
    homeScroll = 0;
  }
}

static void HandlePlantListInput(ButtonEvent_t evt)
{
  if (evt == BTN_EVT_UP && plantIndex > 0) plantIndex--;
  if (evt == BTN_EVT_DOWN && plantIndex < (PLANT_COUNT - 1)) plantIndex++;

  if (evt == BTN_EVT_OK) {
    previewPlantId = plantIndex;
    infoScroll = 0;
    currentScreen = SCREEN_PLANT_INFO;
  }

  if (evt == BTN_EVT_BACK) {
    currentScreen = SCREEN_MENU;
  }
}

static void HandlePlantInfoInput(ButtonEvent_t evt)
{
  if (evt == BTN_EVT_UP && infoScroll > 0) infoScroll--;
  if (evt == BTN_EVT_DOWN && infoScroll < 1) infoScroll++;

  if (evt == BTN_EVT_OK) {
    confirmIndex = 0;
    currentScreen = SCREEN_PLANT_CONFIRM;
  }

  if (evt == BTN_EVT_BACK) {
    currentScreen = SCREEN_PLANT_LIST;
  }
}

static void HandlePlantConfirmInput(ButtonEvent_t evt)
{
  if (evt == BTN_EVT_UP || evt == BTN_EVT_DOWN) {
    confirmIndex = !confirmIndex;
  }

  if (evt == BTN_EVT_OK) {
    if (confirmIndex == 0) {

    	activePlantId = previewPlantId;
    	hasActivePlant = 1;
    	gSensorData.minutesToWater = 80;
    	gSensorData.lightHours = 0.0f;
    	gSensorData.dayRemainSeconds = 24 * 60 * 60;
    	gSensorData.careHours = 0.0f;
    	homeScroll = 0;
    	currentScreen = SCREEN_ACTIVE_HOME;

      History_Add(activePlantId);
      Persist_UpdateFromRuntime();
      Persist_Save();
    } else {
      currentScreen = SCREEN_PLANT_INFO;
    }
  }

  if (evt == BTN_EVT_BACK) {
    currentScreen = SCREEN_PLANT_INFO;
  }
}

static void HandleActiveHomeInput(ButtonEvent_t evt)
{
  if (evt == BTN_EVT_UP && homeScroll > 0) homeScroll--;
  if (evt == BTN_EVT_DOWN && homeScroll < 2) homeScroll++;

  if (evt == BTN_EVT_OK) {
    activeInfoScroll = 0;
    currentScreen = SCREEN_ACTIVE_INFO;
  }

  if (evt == BTN_EVT_BACK) {
    currentScreen = SCREEN_MENU;
  }
}

static void HandleActiveInfoInput(ButtonEvent_t evt)
{
  if (evt == BTN_EVT_UP && activeInfoScroll > 0) activeInfoScroll--;
  if (evt == BTN_EVT_DOWN && activeInfoScroll < 1) activeInfoScroll++;

  if (evt == BTN_EVT_OK || evt == BTN_EVT_BACK) {
    currentScreen = SCREEN_ACTIVE_HOME;
  }
}

static void HandleHistoryListInput(ButtonEvent_t evt)
{
  if (gPersist.historyCount == 0) {
    if (evt == BTN_EVT_BACK || evt == BTN_EVT_OK) {
      currentScreen = SCREEN_MENU;
    }
    return;
  }

  if (evt == BTN_EVT_UP && historyIndex > 0) historyIndex--;
  if (evt == BTN_EVT_DOWN && historyIndex < (gPersist.historyCount - 1) && historyIndex < 3) historyIndex++;

  if (evt == BTN_EVT_OK) {
    currentScreen = SCREEN_HISTORY_DETAIL;
  }

  if (evt == BTN_EVT_BACK) {
    currentScreen = SCREEN_MENU;
  }
}


static void HandleHistoryDetailInput(ButtonEvent_t evt)
{
  if (evt == BTN_EVT_BACK || evt == BTN_EVT_OK) {
    currentScreen = SCREEN_HISTORY_LIST;
  }
}

static void DrawHeader(const char* title)
{
  ssd1306_Fill(Black);
  ssd1306_SetCursor(0, 0);
  ssd1306_WriteString((char*)title, Font_7x10, White);
  ssd1306_Line(0, 11, 127, 11, White);
}

static void DrawMenuScreen(void)
{
  DrawHeader("--MENU--");

  ssd1306_SetCursor(0, 16);
  ssd1306_WriteString(menuIndex == 0 ? "> Chon cay" : "  Chon cay", Font_7x10, White);

  ssd1306_SetCursor(0, 28);
  ssd1306_WriteString(menuIndex == 1 ? "> Lich su" : "  Lich su", Font_7x10, White);
}

static void DrawPlantListScreen(void)
{
  DrawHeader("--CHON CAY--");

  for (uint8_t i = 0; i < PLANT_COUNT; i++) {
    char line[24];
    snprintf(line, sizeof(line), "%c %s", (i == plantIndex) ? '>' : ' ', plants[i].name);
    ssd1306_SetCursor(0, 16 + i * 12);
    ssd1306_WriteString(line, Font_7x10, White);
  }
}

static void DrawPlantInfoCommon(uint8_t plantId, uint8_t scrollValue, const char* header)
{
  char line[32];
  PlantData *p = &plants[plantId];

  DrawHeader(header);

  ssd1306_SetCursor(0, 16);
  ssd1306_WriteString((char*)p->name, Font_7x10, White);

  if (scrollValue == 0) {
    snprintf(line, sizeof(line), "Nhiet:%u-%u", p->tempMin, p->tempMax);
    ssd1306_SetCursor(0, 28);
    ssd1306_WriteString(line, Font_7x10, White);

    snprintf(line, sizeof(line), "Dat :%u-%u%%", p->soilMin, p->soilMax);
    ssd1306_SetCursor(0, 40);
    ssd1306_WriteString(line, Font_7x10, White);

    snprintf(line, sizeof(line), "KKhi:%u-%u%%", p->humidMin, p->humidMax);
    ssd1306_SetCursor(0, 52);
    ssd1306_WriteString(line, Font_7x10, White);
  } else {
    snprintf(line, sizeof(line), "Sang:%u-%uh", p->lightMin, p->lightMax);
    ssd1306_SetCursor(0, 28);
    ssd1306_WriteString(line, Font_7x10, White);

    snprintf(line, sizeof(line), "Tuoi:%u lan/ng", p->waterTimesPerDay);
    ssd1306_SetCursor(0, 40);
    ssd1306_WriteString(line, Font_7x10, White);

    ssd1306_SetCursor(0, 52);
    ssd1306_WriteString("OK:Xac nhan", Font_7x10, White);
  }
}

static void DrawPlantInfoScreen(void)
{
  DrawPlantInfoCommon(previewPlantId, infoScroll, "--THONG SO--");
}

static void DrawPlantConfirmScreen(void)
{
  DrawHeader("--XAC NHAN?--");

  ssd1306_SetCursor(0, 18);
  ssd1306_WriteString((char*)plants[previewPlantId].name, Font_7x10, White);

  ssd1306_SetCursor(0, 34);
  ssd1306_WriteString(confirmIndex == 0 ? "> Co" : "  Co", Font_7x10, White);

  ssd1306_SetCursor(0, 46);
  ssd1306_WriteString(confirmIndex == 1 ? "> Khong" : "  Khong", Font_7x10, White);
}

static void DrawActiveHomeScreen(void)
{
  char line[32];
  PlantData *p = &plants[activePlantId];

  DrawHeader("--DANG CHAM SOC--");

  ssd1306_SetCursor(0, 16);
  ssd1306_WriteString((char*)p->name, Font_7x10, White);

  if (homeScroll == 0)
  {
    snprintf(line, sizeof(line), "Dat :%d%%", gSensorData.soil);
    ssd1306_SetCursor(0, 28);
    ssd1306_WriteString(line, Font_7x10, White);

    snprintf(line, sizeof(line), "KKhi:%d%%", gSensorData.humid);
    ssd1306_SetCursor(0, 40);
    ssd1306_WriteString(line, Font_7x10, White);

    snprintf(line, sizeof(line), "Nhiet:%dC", gSensorData.temp);
    ssd1306_SetCursor(0, 52);
    ssd1306_WriteString(line, Font_7x10, White);
  }
  else if (homeScroll == 1)
  {
    snprintf(line, sizeof(line), "Light:%d%%", gSensorData.lightPercent);
    ssd1306_SetCursor(0, 28);
    ssd1306_WriteString(line, Font_7x10, White);

    int light_int = (int)gSensorData.lightHours;
    int light_dec = (int)(gSensorData.lightHours * 10) % 10;

    snprintf(line, sizeof(line), "Time:%d.%dh", light_int, light_dec);
    ssd1306_SetCursor(0, 40);
    ssd1306_WriteString(line, Font_7x10, White);

    snprintf(line, sizeof(line), "Den:%s",
             gSensorData.lightState ? "ON" : "OFF");
    ssd1306_SetCursor(0, 52);
    ssd1306_WriteString(line, Font_7x10, White);
  }
  else if (homeScroll == 2)
  {
	  int remain_h = gSensorData.dayRemainSeconds / 3600;
	   int remain_m = (gSensorData.dayRemainSeconds % 3600) / 60;
	   int remain_s = gSensorData.dayRemainSeconds % 60;

	   int care_h_int = (int)gSensorData.careHours;
	   int care_h_dec = (int)(gSensorData.careHours * 10.0f) % 10;
	   if (care_h_dec < 0) care_h_dec = -care_h_dec;

	   snprintf(line, sizeof(line), "Con:%02d:%02d:%02d", remain_h, remain_m, remain_s);
	   ssd1306_SetCursor(0, 28);
	   ssd1306_WriteString(line, Font_7x10, White);

	   snprintf(line, sizeof(line), "Cham:%d.%dh", care_h_int, care_h_dec);
	   ssd1306_SetCursor(0, 40);
	   ssd1306_WriteString(line, Font_7x10, White);

	   int speed_int = (int)gTimeScale;
	   int speed_dec = (int)(gTimeScale * 10.0f) % 10;
	   if (speed_dec < 0) speed_dec = -speed_dec;

	   snprintf(line, sizeof(line), "Speed:%d.%dx", speed_int, speed_dec);
	   ssd1306_SetCursor(0, 52);
	   ssd1306_WriteString(line, Font_7x10, White);
  }
}

static void DrawActiveInfoScreen(void)
{
  DrawPlantInfoCommon(activePlantId, activeInfoScroll, "--CAY HIEN TAI--");
}

static void DrawHistoryListScreen(void)
{
  DrawHeader("--LICH SU--");

  if (gPersist.historyCount == 0) {
    ssd1306_SetCursor(0, 20);
    ssd1306_WriteString("Chua co lich su", Font_7x10, White);
    return;
  }

  for (uint8_t i = 0; i < gPersist.historyCount && i < 4; i++) {
    int realIndex = (gPersist.historyWriteIndex + HISTORY_MAX - 1 - i) % HISTORY_MAX;

    if (gPersist.history[realIndex].used) {
      char line[24];
      uint8_t pid = gPersist.history[realIndex].plantId;
      snprintf(line, sizeof(line), "%c %s", (i == historyIndex) ? '>' : ' ', plants[pid].name);

      ssd1306_SetCursor(0, 16 + i * 12);
      ssd1306_WriteString(line, Font_7x10, White);
    }
  }
}

static void DrawHistoryDetailScreen(void)
{
  DrawHeader("--CHI TIET--");

  if (gPersist.historyCount == 0) {
    ssd1306_SetCursor(0, 20);
    ssd1306_WriteString("Chua co lich su", Font_7x10, White);
    return;
  }

  int realIndex = (gPersist.historyWriteIndex + HISTORY_MAX - 1 - historyIndex) % HISTORY_MAX;
  uint8_t pid = gPersist.history[realIndex].plantId;
  PlantData *p = &plants[pid];

  ssd1306_SetCursor(0, 16);
  ssd1306_WriteString((char*)p->name, Font_7x10, White);

  char line[24];

  snprintf(line, sizeof(line), "Nhiet:%u-%u", p->tempMin, p->tempMax);
  ssd1306_SetCursor(0, 28);
  ssd1306_WriteString(line, Font_7x10, White);

  snprintf(line, sizeof(line), "Dat:%u-%u%%", p->soilMin, p->soilMax);
  ssd1306_SetCursor(0, 40);
  ssd1306_WriteString(line, Font_7x10, White);

  snprintf(line, sizeof(line), "Tuoi:%u lan", p->waterTimesPerDay);
  ssd1306_SetCursor(0, 52);
  ssd1306_WriteString(line, Font_7x10, White);
}
static void RenderScreen(void)
{
  osMutexAcquire(appMutexHandle, osWaitForever);
  osMutexAcquire(oledMutexHandle, osWaitForever);

  switch (currentScreen) {
    case SCREEN_MENU:
      DrawMenuScreen();
      break;
    case SCREEN_PLANT_LIST:
      DrawPlantListScreen();
      break;
    case SCREEN_PLANT_INFO:
      DrawPlantInfoScreen();
      break;
    case SCREEN_PLANT_CONFIRM:
      DrawPlantConfirmScreen();
      break;
    case SCREEN_ACTIVE_HOME:
      DrawActiveHomeScreen();
      break;
    case SCREEN_ACTIVE_INFO:
      DrawActiveInfoScreen();
      break;
    case SCREEN_HISTORY_LIST:
      DrawHistoryListScreen();
      break;
    case SCREEN_HISTORY_DETAIL:
      DrawHistoryDetailScreen();
      break;
    default:
      DrawMenuScreen();
      break;
  }

  ssd1306_UpdateScreen();

  osMutexRelease(oledMutexHandle);
  osMutexRelease(appMutexHandle);
}

void Error_Blink(uint8_t count)
{
  __disable_irq();
  while (1)
  {
    for (uint8_t i = 0; i < count; i++)
    {
      HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
      for (volatile uint32_t d = 0; d < 500000; d++);

      HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);
      for (volatile uint32_t d = 0; d < 500000; d++);
    }


    for (volatile uint32_t d = 0; d < 3000000; d++);
  }
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_I2C1_Init();
  MX_ADC1_Init();
  MX_TIM3_Init();
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */
  Persist_Load();
  Persist_ApplyToRuntime();


  /* USER CODE END 2 */

  /* Init scheduler */
  osKernelInitialize();

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  appMutexHandle = osMutexNew(&appMutex_attributes);
  if (appMutexHandle == NULL) Error_Blink(1);

  oledMutexHandle = osMutexNew(&oledMutex_attributes);
  if (oledMutexHandle == NULL) Error_Blink(2);

  buttonQueueHandle = osMessageQueueNew(8, sizeof(ButtonEvent_t), NULL);
  if (buttonQueueHandle == NULL) Error_Blink(3);

  uiEventHandle = osEventFlagsNew(NULL);
  if (uiEventHandle == NULL) Error_Blink(4);
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of defaultTask */
  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  inputTaskHandle = osThreadNew(StartInputTask, NULL, &inputTask_attributes);
  if (inputTaskHandle == NULL) Error_Blink(6);

  sensorTaskHandle = osThreadNew(StartSensorTask, NULL, &sensorTask_attributes);
  if (sensorTaskHandle == NULL) Error_Blink(7);

  logicTaskHandle = osThreadNew(StartLogicTask, NULL, &logicTask_attributes);
  if (logicTaskHandle == NULL) Error_Blink(8);

  displayTaskHandle = osThreadNew(StartDisplayTask, NULL, &displayTask_attributes);
  if (displayTaskHandle == NULL) Error_Blink(9);

  dhtTaskHandle = osThreadNew(StartDHTTask, NULL, &dhtTask_attributes);
  if (dhtTaskHandle == NULL) Error_Blink(15);
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

  /* Start scheduler */
  osKernelStart();

  /* We should never get here as control is now taken by the scheduler */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC;
  PeriphClkInit.AdcClockSelection = RCC_ADCPCLK2_DIV6;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief ADC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC1_Init(void)
{

  /* USER CODE BEGIN ADC1_Init 0 */

  /* USER CODE END ADC1_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC1_Init 1 */

  /* USER CODE END ADC1_Init 1 */

  /** Common config
  */
  hadc1.Instance = ADC1;
  hadc1.Init.ScanConvMode = ADC_SCAN_DISABLE;
  hadc1.Init.ContinuousConvMode = DISABLE;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.NbrOfConversion = 1;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_4;
  sConfig.Rank = ADC_REGULAR_RANK_1;
  sConfig.SamplingTime = ADC_SAMPLETIME_1CYCLE_5;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */

  /* USER CODE END ADC1_Init 2 */

}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 100000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief TIM3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM3_Init(void)
{

  /* USER CODE BEGIN TIM3_Init 0 */

  /* USER CODE END TIM3_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM3_Init 1 */

  /* USER CODE END TIM3_Init 1 */
  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 71;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 65535;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim3, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM3_Init 2 */

  /* USER CODE END TIM3_Init 2 */

}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13|GPIO_PIN_14, GPIO_PIN_RESET);

  /*Configure GPIO pin : PC13 */
  GPIO_InitStruct.Pin = GPIO_PIN_13;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : PA0 PA1 PA2 PA3 */
  GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : PB12 */
  GPIO_InitStruct.Pin = GPIO_PIN_12;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : PB13 PB14 */
  GPIO_InitStruct.Pin = GPIO_PIN_13|GPIO_PIN_14;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
void StartInputTask(void *argument)
{
  uint8_t sta_now[4] = {0};
  uint8_t sta_pre[4] = {0};
  uint8_t sta_final[4] = {0};
  uint8_t stable_last[4] = {0};
  uint8_t flag_press[4] = {0};
  uint32_t time_flag[4] = {0};

  for (;;) {
    ProcessButton(BTN_OK_PORT, BTN_OK_PIN, BTN_EVT_OK,
                  &sta_now[0], &sta_pre[0], &sta_final[0], &stable_last[0], &flag_press[0], &time_flag[0]);

    ProcessButton(BTN_BACK_PORT, BTN_BACK_PIN, BTN_EVT_BACK,
                  &sta_now[1], &sta_pre[1], &sta_final[1], &stable_last[1], &flag_press[1], &time_flag[1]);

    ProcessButton(BTN_UP_PORT, BTN_UP_PIN, BTN_EVT_UP,
                  &sta_now[2], &sta_pre[2], &sta_final[2], &stable_last[2], &flag_press[2], &time_flag[2]);

    ProcessButton(BTN_DOWN_PORT, BTN_DOWN_PIN, BTN_EVT_DOWN,
                  &sta_now[3], &sta_pre[3], &sta_final[3], &stable_last[3], &flag_press[3], &time_flag[3]);

    osDelay(10);
  }
}

void StartDHTTask(void *argument)
{
  uint8_t ok;

  DHT_Init(&DHT1, DHT22, &htim3, GPIOB, GPIO_PIN_12);

  for (;;)
  {
    __disable_irq();
    ok = DHT_ReadTempHum(&DHT1);
    __enable_irq();

    osMutexAcquire(appMutexHandle, osWaitForever);

    /* Luon cap nhat gia tri hien thi bang gia tri moi nhat trong DHT1 */
    if (DHT1.Temp >= 0.0f)
      gSensorData.temp = (int)(DHT1.Temp + 0.5f);
    else
      gSensorData.temp = (int)(DHT1.Temp - 0.5f);

    gSensorData.humid = (int)(DHT1.Humi + 0.5f);

    gDhtTempCache = DHT1.Temp;
    gDhtHumidCache = DHT1.Humi;
    gDhtHasValidData = 1;

    if (ok) {
      dhtOk = 1;
      dhtValidCount++;
    } else {
      dhtOk = 0;
    }

    osMutexRelease(appMutexHandle);

    RequestRedraw();
    osDelay(2000);
  }
}


void StartSensorTask(void *argument)
{
  for (;;) {
	  UpdateSensorData();
	  char msg[64];

	  snprintf(msg, sizeof(msg),
	           "SOIL:%d,TEMP:%d,HUM:%d,LIGHT:%d\r\n",
	           gSensorData.soil,
	           gSensorData.temp,
	           gSensorData.humid,
	           gSensorData.lightPercent);

	  HAL_UART_Transmit(&huart1, (uint8_t*)msg, strlen(msg), 100);
    RequestRedraw();
    osDelay(1000);
  }
}

void StartLogicTask(void *argument)
{
  ButtonEvent_t evt;
  uint32_t lastActiveRefreshTick = osKernelGetTickCount();

  osMutexAcquire(appMutexHandle, osWaitForever);
  if (hasActivePlant) {
    currentScreen = SCREEN_ACTIVE_HOME;
  } else {
    currentScreen = SCREEN_MENU;
  }
  menuIndex = 0;
  osMutexRelease(appMutexHandle);

  RequestRedraw();

  for (;;) {
    if (osMessageQueueGet(buttonQueueHandle, &evt, NULL, 20) == osOK) {
      osMutexAcquire(appMutexHandle, osWaitForever);

      switch (currentScreen) {
        case SCREEN_MENU:
          HandleMenuInput(evt);
          break;
        case SCREEN_PLANT_LIST:
          HandlePlantListInput(evt);
          break;
        case SCREEN_PLANT_INFO:
          HandlePlantInfoInput(evt);
          break;
        case SCREEN_PLANT_CONFIRM:
          HandlePlantConfirmInput(evt);
          break;
        case SCREEN_ACTIVE_HOME:
          HandleActiveHomeInput(evt);
          break;
        case SCREEN_ACTIVE_INFO:
          HandleActiveInfoInput(evt);
          break;
        case SCREEN_HISTORY_LIST:
          HandleHistoryListInput(evt);
          break;
        case SCREEN_HISTORY_DETAIL:
          HandleHistoryDetailInput(evt);
          break;
        default:
          currentScreen = SCREEN_MENU;
          break;
      }

      osMutexRelease(appMutexHandle);
      RequestRedraw();
    }

    if ((osKernelGetTickCount() - lastActiveRefreshTick) >= ACTIVE_HOME_REFRESH_MS) {
      lastActiveRefreshTick = osKernelGetTickCount();

      osMutexAcquire(appMutexHandle, osWaitForever);
      if (currentScreen == SCREEN_ACTIVE_HOME) {
        osMutexRelease(appMutexHandle);
        RequestRedraw();
      } else {
        osMutexRelease(appMutexHandle);
      }
    }
  }
}

void StartDisplayTask(void *argument)
{
  osDelay(50);

  ssd1306_Init();
  ssd1306_Fill(Black);
  ssd1306_UpdateScreen();

  for (;;) {
    osEventFlagsWait(uiEventHandle, EVT_REDRAW, osFlagsWaitAny, osWaitForever);
    RenderScreen();
  }
}
/* USER CODE END 4 */

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void *argument)
{
  /* USER CODE BEGIN 5 */
  /* Infinite loop */
	  for (;;)
	  {
	    osDelay(osWaitForever);
	  }
  /* USER CODE END 5 */
}

/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM4 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM4)
  {
    HAL_IncTick();
  }
  /* USER CODE BEGIN Callback 1 */

  /* USER CODE END Callback 1 */
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
	  HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
	     for (volatile uint32_t i = 0; i < 200000; i++);
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
