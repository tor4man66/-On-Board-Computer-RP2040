// =====================================================================================================================
// --- Config (Config.h) ---
// =====================================================================================================================
#ifndef CONFIG_H
#define CONFIG_H
#include <cstddef>
#include <cstdint>

// =====================================================================================================================
// 0. СИСТЕМНІ РЕЖИМИ
// =====================================================================================================================
enum SystemMode {
    NORMAL_MODE,    // Звичайний режим роботи бортового комп'ютера
    SERVICE_MODE    // Сервісний режим для діагностики K-Line
};

// =====================================================================================================================
// 1. АКТИВАЦІЯ ФУНКЦІЙ ТА МОДУЛІВ
// =====================================================================================================================
#define ENABLE_SERVICE_MODE                        true  // Увімкнути/вимкнути сервісний режим
#define SERVICE_MODE_ACC_PIN_CHECK                 true  // Увімкнути/вимкнути перевірку ACC у сервісному режимі
#define KLINE_ENABLE                               true  // Увімкнути/вимкнути весь функціонал K-Line (підключення до ЕБУ, читання/скидання помилок)
#define ENABLE_OLED                                true  // Увімкнути/вимкнути OLED-дисплей (повністю відключає рендеринг)
#define ENABLE_SPEAKER                             true  // Увімкнути/вимкнути динамік для звукових сповіщень (зумер)
#define ENABLE_WATCHDOG_TIMER                      true  // Увімкнути/вимкнути апаратний сторожовий таймер для захисту від зависань
#define ENABLE_PIN_INJ                             true  // Увімкнути/вимкнути моніторинг піна форсунки (основа для розрахунку витрати палива)
#define ENABLE_PIN_VSS                             true  // Увімкнути/вимкнути моніторинг датчика швидкості (VSS) для визначення пробігу та швидкості
#define ENABLE_PIN_IGNITION_RPM                    true  // Увімкнути/вимкнути моніторинг піна запалювання для вимірювання обертів двигуна (RPM)
#define ENABLE_PIN_BUTTON_RESET                    true  // Увімкнути/вимкнути кнопку скидання TRIP та перемикання режимів дисплея
#define ENABLE_PIN_SENSOR_BRAKE_FLUID              true  // Увімкнути/вимкнути датчик рівня гальмівної рідини (замикання на землю при низькому рівні)
#define ENABLE_PIN_SENSOR_OIL_PRESSURE_0_3         true  // Увімкнути/вимкнути датчик низького тиску мастила (0.3 бар, замикання на землю)
#define ENABLE_PIN_SENSOR_OVERHEAT_AND_LOW_COOLANT true  // Увімкнути/вимкнути датчик перегріву або низького рівня охолоджувальної рідини
#define ENABLE_PIN_SENSOR_OIL_PRESSURE_1_8         true  // Увімкнути/вимкнути датчик високого тиску мастила (1.8 бар, розмикання на землю)
#define ENABLE_PIN_FUEL_LEVEL_ADC                  true  // Увімкнути/вимкнути аналоговий датчик рівня палива (ADC)
#define ENABLE_PIN_BOARD_VOLTAGE_ADC               true  // Увімкнути/вимкнути вимірювання напруги бортової мережі через дільник (ADC)
#define ENABLE_PIN_BRIGHTNESS_ADC                  true  // Увімкнути/вимкнути датчик зовнішньої яскравості для авторегулювання контрасту OLED (ADC)
#define ENABLE_PIN_ACC                             true  // Увімкнути/вимкнути моніторинг піна ACC (сигнал "замок запалювання" для входу/виходу зі сну)

// =====================================================================================================================
// 2. КОНФІГУРАЦІЯ GPIO ПІНІВ
// =====================================================================================================================
// --- Піни датчиків двигуна ---
#define PIN_INJ                                   12    // Вхід з форсунки: імпульси для розрахунку витрати палива
#define PIN_VSS                                   10    // Вхід з датчика швидкості (Vehicle Speed Sensor): імпульси на кілометр
#define PIN_IGNITION_RPM                          14    // Вхід з котушки запалювання: імпульси для визначення обертів двигуна

// --- Піни I2C для OLED ---
#define PIN_I2C_SDA                               16    // Лінія даних I2C (SDA) для OLED-дисплея
#define PIN_I2C_SCL                               17    // Лінія тактування I2C (SCL) для OLED-дисплея

// --- Піни кнопок ---
#define PIN_BUTTON_RESET                          8     // Кнопка скидання TRIP / перемикання режимів дисплея
#define SERVICE_MODE_BUTTON_PIN                   8     // Пін кнопки входу в сервісний режим

// --- Піни цифрових датчиків ---
#define PIN_SENSOR_BRAKE_FLUID                    6     // Датчик рівня гальмівної рідини (LOW = низький рівень)
#define PIN_SENSOR_OIL_PRESSURE_0_3               4     // Датчик тиску мастила 0.3 бар (LOW = тиск нижче порогу)
#define PIN_SENSOR_OVERHEAT_AND_LOW_COOLANT       3     // Датчик перегріву / низького рівня антифризу
#define PIN_SENSOR_OIL_PRESSURE_1_8               1     // Датчик тиску мастила 1.8 бар (HIGH = тиск вище порогу при RPM > 2000)
#define PIN_ACC                                   28    // Сигнал ACC (Accessory): HIGH = запалювання увімкнено, LOW = вимкнено

// --- Піни виконавчих пристроїв ---
#define PIN_SPEAKER                               19    // Вихід на динамік (зумер) для звукових сповіщень
#define KLINE_CHECK_ENGINE_LIGHT_PIN              15    // Вихід на лампочку "Check Engine" (засвічується при помилках K-Line)

// --- Піни аналогових датчиків (ADC) ---
#define PIN_FUEL_LEVEL_ADC                        26    // Аналоговий вхід з датчика рівня палива (потенціометр у баку)
#define PIN_ADC                                   29    // Аналоговий вхід з дільника напруги бортової мережі
#define PIN_BRIGHTNESS_ADC                        27    // Аналоговий вхід з фоторезистора для автояскравості OLED

// --- Піни K-Line ---
#define KLINE_TX_PIN                              20    // TX для K-Line (передача даних до ЕБУ)
#define KLINE_RX_PIN                              21    // RX для K-Line (прийом даних від ЕБУ)

// =====================================================================================================================
// 3. НАЛАШТУВАННЯ СИСТЕМНОЇ ЛОГІКИ ТА ТАЙМІНГІВ
// =====================================================================================================================
// --- Режим сну (Dormant Mode) ---
#define DORMANT_WAIT_TIMEOUT_MS                   15000UL // Час очікування (мс) після вимкнення ACC до примусового переходу в режим сну
#define DORMANT_MIN_GRACE_PERIOD_MS               12000UL // Мінімальний час (мс), який система чекає готовності дисплея завершити роботу перед сном

// --- Основні цикли ---
#define UPDATE_INTERVAL_MS                        1000UL  // Інтервал (мс) оновлення даних моніторів (двигун, паливо, напруга)
#define UPDATE_INTERVAL_SEC                       1.0f    // Інтервал у секундах для зручності float-обчислень
#define MAIN_LOOP_IDLE_SLEEP_MS                   1       // Затримка (мс) у головному циклі loop() для зниження навантаження на CPU

// --- Логіка двигуна ---
#define ENGINE_STOP_TIMEOUT_MS                    2000UL  // Час (мс) без імпульсів форсунки, після якого двигун вважається зупиненим
#define MIN_RPM_FOR_STABLE_RUNNING                550     // Мінімальні оберти (RPM), що вважаються "стабільною роботою двигуна"
#define ENGINE_RPM_STABILITY_HYSTERESIS_MS        2000UL  // Час (мс) утримання RPM вище порогу для підтвердження стабільної роботи

// --- Режим буксирування (Towing Mode) ---
#define TOWING_MODE_MIN_VSS_INACTIVITY_MS         1000UL  // Час (мс) без імпульсів VSS, після якого VSS вважається неактивним
#define TOWING_MODE_MAX_RPM                       100     // Максимальні оберти (RPM), при яких можлива активація режиму буксирування
#define TOWING_MODE_MIN_SPEED_KMH                 5.0f    // Мінімальна швидкість (км/год) для активації режиму буксирування

// --- Збереження даних ---
#define PERSISTENT_SAVE_INTERVAL_MS               90000UL // Інтервал (мс) автоматичного збереження TRIP/PERS даних в EEPROM

// --- Сторожовий таймер ---
#define WATCHDOG_TIME_RESET                       15000UL // Максимальний час (мс) без "годування" собаки до перезавантаження системи

// --- Налагодження (DEBUG) ---
#define DEBUG_ENABLED                             true    // Глобальний рубильник: вмикає/вимикає всі DEBUG-повідомлення в Serial
#define DEBUG_INITIALIZATION_ENABLED              true    // Вивід у Serial деталей процесу ініціалізації системи
#define KLINE_DEBUG_INFO_ENABLED                  true    // Вивід у Serial статусної інформації K-Line (підключення, відключення)
#define KLINE_DEBUG_TRAFFIC_ENABLED               true    // Вивід у Serial повного трафіку K-Line (кожен байт TX/RX)

// =====================================================================================================================
// 4. НАЛАШТУВАННЯ ДИСПЛЕЯ (OLED)
// =====================================================================================================================
// --- Апаратні параметри ---
#define OLED_RESET_PIN                            -1       // Пін апаратного скидання OLED (-1 якщо не підключений)
#define I2C_FREQ                                  400000UL // Частота шини I2C у Гц (400 кГц — стандарт для SH1107/SSD1306)
#define OLED_ADDR_HEX                             0x3C     // I2C-адреса дисплея (0x3C — найтиповіша, іноді 0x3D)
#define OLED_WIDTH                                128      // Ширина дисплея в пікселях
#define OLED_HEIGHT                               128      // Висота дисплея в пікселях
#define OLED_CONTRAST                             0xFF     // Початковий рівень контрасту (0x00 = мінімум, 0xFF = максимум)
#define OLED_CONTRAST_MIN                         0x00     // Мінімально можливий контраст (повністю темний екран)
#define OLED_CONTRAST_MAX                         0xFF     // Максимально можливий контраст

// --- Шрифти ---
#define FONT_WIDTH_PX                             8        // Ширина символу стандартного шрифту (5x7 з проміжками ≈ 8 пікселів)
#define FONT_HEIGHT_PX                            8        // Висота символу стандартного шрифту (7 + 1 проміжок = 8 пікселів)
#define DEFAULT_TEXT_NONE                         "NO"     // Текст, що показується, коли дані недоступні

// --- Одиниці виміру ---
#define UNIT_TEXT_LITERS                          "L"      // Літри (для палива)
#define UNIT_TEXT_KM                              "KM"     // Кілометри (для відстані)
#define UNIT_TEXT_LH                              "L/H"    // Літри на годину (миттєва витрата)
#define UNIT_TEXT_L100KM                          "L/100KM"// Літри на 100 км (середня витрата)
#define UNIT_TEXT_LH2                             "Л/ГОД"  // Український варіант: літри на годину (головний екран)
#define UNIT_TEXT_L100KM2                         "Л/100КМ"// Український варіант: літри на 100 км (головний екран)
#define UNIT_TEXT_KMH                             "KMH"    // Кілометри на годину (швидкість)
#define UNIT_TEXT_VOLTS                           "V"      // Вольти (напруга)
#define UNIT_TEXT_RPM                             "RPM"    // Оберти на хвилину
#define UNIT_TEXT_MS                              "MS"     // Мілісекунди (тривалість імпульсу форсунки)

// --- Анімація ---
#define STARTUP_OK_SCREEN_DURATION_MS             3000UL   // Скільки мс показувати екран "СТАТУС ОК" при успішному запуску
#define BLINK_INTERVAL_MS                         1000UL   // Період блимання (мс)
#define ERROR_ANIMATION_SPEED                     100      // Затримка між кадрами анімації іконок помилок (мс)

// --- Екран критичної помилки ---
#define LOOP_ERROR_TEXT_SIZE                      3        // Розмір шрифту для напису "ERROR" при зависанні loop()
#define LOOP_ERROR_X_POS                          15       // X-координата тексту помилки циклу
#define LOOP_ERROR_Y_POS                          40       // Y-координата тексту помилки циклу
#define LOOP_ERROR_TEXT                           "ERROR"  // Текст, що виводиться при критичній помилці головного циклу
#define LOOP_ERROR_PAUSE_MS                       15000UL  // Час (мс) показу помилки перед спробою відновлення

// --- Режими ---
#define DEFAULT_MAIN_DISPLAY_MODE                 0        // Який екран показувати при старті 
#define MAX_MAIN_DISPLAY_MODES                    3        // Загальна кількість екранів

// =====================================================================================================================
// 5. КОНФІГУРАЦІЯ СПОВІЩЕНЬ ТА СЕНСОРНИХ ПОРОГІВ
// =====================================================================================================================
// --- Рівні критичності ---
#define ERROR_SEVERITY_NONE                       0        // Помилок немає
#define ERROR_SEVERITY_NON_CRITICAL               1        // Некритична помилка
#define CRITICAL_ERROR_SEVERITY                   2        // Критична помилка

// --- Логічні рівні датчиків ---
#define SENSOR_ACTIVE_LOW_VALUE                   0        // Датчик активний, коли на піні LOW
#define SENSOR_ACTIVE_HIGH_VALUE                  1        // Датчик активний, коли на піні HIGH

// --- Пороги спрацювання ---
#define OIL_CHECK_DELAY_MS                        5000UL   // Затримка (мс) після запуску двигуна перед першою перевіркою тиску мастила
#define MIN_RPM_FOR_HIGH_PRESSURE_CHECK           2000     // Мінімальні оберти для перевірки датчика високого тиску (1.8 бар)
#define NO_CHARGE_THRESHOLD_VOLTAGE               13.0f    // Якщо напруга нижча за цю при працюючому двигуні — "немає зарядки"
#define FUEL_LOW_THRESHOLD_PERCENT                6.0f     // Поріг "мало палива" (% від об'єму бака)
#define FUEL_HIGH_THRESHOLD_PERCENT               12.0f    // Гістерезис: попередження зникне тільки коли рівень підніметься вище цього %

// --- Таймінги циклів помилок ---
#define ERROR_DISPLAY_CYCLE_MS                     3000UL   // Час (мс) показу однієї іконки помилки перед перемиканням на наступну
#define NON_CRITICAL_ERROR_ICON_DURATION_MS        5000UL   // Час (мс) показу іконки некритичної помилки
#define NON_CRITICAL_ERROR_MAIN_SCREEN_DURATION_MS 20000UL  // Час (мс) показу основного екрана між показами іконки некритичної помилки

// --- Звукова сигналізація ---
typedef struct {
    unsigned long duration_ms;  // Тривалість фази (мс)
    unsigned int  frequency_hz; // Частота звуку (Гц), 0 = тиша
} AlarmPhase;

const AlarmPhase ALARM_SEQUENCE[] = {  // Послідовність "біп-біп-біп — пауза" для привертання уваги
    {1000UL, 1000U},                   // 1 сек: звук 1000 Гц
    {1000UL, 0U},                      // 1 сек: тиша
    {1000UL, 1000U},                   // 1 сек: звук
    {1000UL, 0U},                      // 1 сек: тиша
    {1000UL, 1000U},                   // 1 сек: звук
    {15000UL, 0U}                      // 15 сек: довга пауза перед повторенням
};
const size_t ALARM_SEQUENCE_SIZE = sizeof(ALARM_SEQUENCE) / sizeof(ALARM_SEQUENCE[0]);

// =====================================================================================================================
// 6. НАЛАШТУВАННЯ КНОПКИ
// =====================================================================================================================
// --- Загальні параметри ---
#define BUTTON_SHORT_PRESS_MAX_MS                   1000UL   // Максимальна тривалість (мс), яка вважається "коротким натисканням"
#define BUTTON_DISPLAY_MODE_SWITCH_BEEP_FREQ        1500U    // Частота звуку (Гц) при перемиканні екрану коротким натисканням
#define BUTTON_DISPLAY_MODE_SWITCH_BEEP_DURATION_MS 50UL     // Тривалість звуку (мс) при перемиканні екрану

// --- Режим 0 ---
#define ACTION_RESET_TRIP_STR                     "reset_trip"          // Команда: скинути лічильники TRIP
#define ACTION_DO_NOTHING_STR                     "do_nothing"          // Команда: нічого не робити
#define ACTION_CUSTOM_FUNCTION_A_STR              "custom_function_A"   // Текст дії: виклик користувацької функції А
#define MODE0_LONG_PRESS_ACTION                   ACTION_RESET_TRIP_STR // Довге натискання в режимі 0 скидає TRIP
#define BUTTON_TRIP_RESET_HOLD_MS                 3000UL                // Скільки мс утримувати кнопку для скидання TRIP
#define BUTTON_TRIP_RESET_BEEP_FREQ               1500U                  // Частота звуку (Гц) при скиданні TRIP
#define BUTTON_TRIP_RESET_BEEP_DURATION_MS        100UL                 // Тривалість звуку (мс) при скиданні TRIP

// --- Режим 1 ---
#define MODE1_LONG_PRESS_ACTION                   ACTION_DO_NOTHING_STR // Довге натискання в режимі 1 нічого не робить
#define MODE1_BUTTON_TRIP_RESET_HOLD_MS           3000UL                // (Зарезервовано)
#define MODE1_BUTTON_TRIP_RESET_BEEP_FREQ         1500U                 // (Зарезервовано)
#define MODE1_BUTTON_TRIP_RESET_BEEP_DURATION_MS  100UL                 // (Зарезервовано)

// --- Режим 2 (K-Line DTC) ---
#define MODE2_LONG_PRESS_ACTION                   "clear_faults_kline" // Довге натискання в режимі 2 скидає помилки ЕБУ
#define MODE2_BUTTON_HOLD_MS                      3000UL               // Скільки мс утримувати кнопку для скидання помилок
#define MODE2_BUTTON_BEEP_FREQ                    1500U                // Частота звуку (Гц) при старті очищення помилок
#define MODE2_BUTTON_BEEP_DURATION_MS             100UL                // Тривалість звуку (мс) при старті очищення

// =====================================================================================================================
// 7. КАЛІБРУВАННЯ ДАТЧИКІВ ТА РОЗРАХУНКОВІ ПАРАМЕТРИ
// =====================================================================================================================
// --- Форсунка ---
#define INJ_FLOW_RATE_ML_PER_MIN                  344.0f  // Продуктивність форсунки (мл/хв) — основа розрахунку витрати палива
#define INJ_DEAD_TIME_US                          150     // "Мертвий час" форсунки (мкс) — затримка між сигналом і реальним відкриттям
#define INJ_VOLT_SENSITIVITY                      10      // На скільки мкс змінюється мертвий час при зміні напруги на 1В
#define MIN_VOLTAGE_FOR_DEADTIME_CORR             8.0f    // Нижче цієї напруги (В) корекція мертвого часу не застосовується
#define REFERENCE_VOLTAGE_FOR_DEADTIME_CORR       14.0f   // Еталонна напруга (В), при якій мертвий час дорівнює INJ DEAD TIME US
#define MAX_DYNAMIC_DEAD_TIME_US                  500     // Максимальне значення динамічного мертвого часу (мкс) після корекції
#define MIN_INJ_PULSE_WIDTH_FILTER_US             500     // Імпульси коротші за це (мкс) ігноруються як шум
#define INJ_IRQ_DEBOUNCE_US                       100     // Антидребезг для переривання форсунки (мкс)

// --- Датчик швидкості (VSS) ---
#define VSS_IMPULSES_PER_KM                       4324    // Скільки імпульсів VSS припадає на 1 км пробігу
#define VSS_DEBOUNCE_US                           500     // Антидребезг для переривання VSS (мкс)

// --- Датчик рівня палива ---
#define FUEL_TANK_CAPACITY_L                      68.0f                                      // Повний об'єм паливного бака в літрах
#define FUEL_ADC_MIN_RAW_SCALED                   (200.0f * (ADC_MAX_RAW_VALUE / 65535.0f))  // Сире ADC при порожньому баку
#define FUEL_ADC_MAX_RAW_SCALED                   (1950.0f * (ADC_MAX_RAW_VALUE / 65535.0f)) // Сире ADC при повному баку
#define FUEL_ADC_READINGS_FOR_MEDIAN              8                                          // Скільки значень ADC зчитувати для медіанного фільтра
#define FUEL_ADC_MEDIAN_INDEX                     3                                          // Індекс медіани в сортованому масиві
#define FUEL_BUFFER_SIZE                          16                                         // Розмір буфера для експоненційного згладжування рівня палива
#define FUEL_MAX_PERCENT_CHANGE_PER_SEC           10.0f                                      // Максимальна допустима зміна рівня палива (%/с) для ігнорування сплесків
#define FUEL_SMOOTHING_MIN_EFFECTIVE_INTERVAL_SEC 0.5f                                       // Мінімальний інтервал (с) для коректного згладжування

// --- Тахометр (RPM) ---
#define RPM_BASE_FACTOR                           60000000UL // Константа для перетворення періоду (мкс) в оберти/хв (60 * 1_000_000)
#define RPM_PULSES_PER_ENGINE_REVOLUTION          2          // Скільки імпульсів запалювання на один оберт двигуна
#define MIN_IGNITION_PULSE_PERIOD_US              7000       // Мінімальний період між імпульсами (мкс) — відповідає ~8500 RPM
#define MAX_IGNITION_PULSE_PERIOD_US              120000     // Максимальний період (мкс) — відповідає ~500 RPM
#define MIN_DISPLAY_RPM                           400        // Нижче цих обертів на дисплеї показується "----"
#define MAX_DISPLAY_RPM                           8500       // Максимальні оберти, що можуть бути показані
#define IGNITION_RPM_IRQ_DEBOUNCE_US              100        // Антидребезг для переривання запалювання (мкс)

// --- ADC ---
#define ADC_MAX_RAW_VALUE                         4095    // Максимальне значення 12-бітного АЦП (2^12 - 1)
#define ADC_REF_VOLTAGE                           3.3f    // Опорна напруга АЦП (В) на Raspberry Pi Pico

// --- Автояскравість ---
#define BRIGHTNESS_ADC_FULL_RANGE_VALUE           65535UL                                     // Значення АЦП для масштабування (16-бітний діапазон)
#define BRIGHTNESS_ADC_NO_VOLTAGE_THRESHOLD_RAW   (700.0f * (ADC_MAX_RAW_VALUE / 65535.0f))   // Мінімальне значення для максимальної яскравості
#define BRIGHTNESS_ADC_MIN_VOLTAGE_RAW            (2000.0f * (ADC_MAX_RAW_VALUE / 65535.0f))  // Значення, що відповідає мінімальній яскравості
#define BRIGHTNESS_ADC_MAX_VOLTAGE_RAW            (36000.0f * (ADC_MAX_RAW_VALUE / 65535.0f)) // Значення, що відповідає максимальній яскравості
#define BRIGHTNESS_CONTRAST_HYSTERESIS_VALUE      5                                           // Гістерезис контрасту (одиниць) для запобігання мерехтінню
#define BRIGHTNESS_ADC_READ_INTERVAL_MS           2000UL                                      // Інтервал (мс) між вимірюваннями яскравості

// --- Напруга бортової мережі ---
#define VOLTAGE_R1                                10000.0f                                                                         // Опір верхнього плеча дільника напруги (Ом)
#define VOLTAGE_R2                                2200.0f                                                                          // Опір нижнього плеча дільника напруги (Ом)
#define VOLTAGE_CALIBRATION_FACTOR                (ADC_REF_VOLTAGE / ADC_MAX_RAW_VALUE) * ((VOLTAGE_R1 + VOLTAGE_R2) / VOLTAGE_R2) // Коефіцієнт перетворення ADC → Вольти

// --- Пороги для формул ---
#define MIN_INTERVAL_FOR_SPEED_CALC               0.001f  // Мінімальний інтервал (с) для ділення при розрахунку швидкості
#define MIN_INTERVAL_FOR_VOLUME_CALC              0.001f  // Мінімальний інтервал (с) для ділення при розрахунку об'єму палива
#define STATIONARY_THRESHOLD                      0.05f   // Витрата (Л/год) нижче якої двигун вважається на холостому (показується "-.--")

// --- Статистика TRIP ---
#define MAX_TRIP_LITERS                           99.9f   // Максимальне значення лічильника витраченого палива
#define MAX_TRIP_DISTANCE                         999     // Максимальне значення лічильника пробігу
#define MIN_SPEED_FOR_L100KM_KMH                  10.0f   // Мінімальна швидкість (км/год) для початку розрахунку середньої витрати
#define MIN_DISTANCE_FOR_L100KM_KM                0.1f    // Мінімальна відстань (км) для показу середньої витрати
#define MIN_DISTANCE_FOR_L100KM_CALC              0.0001f // Мінімальна відстань (км) для уникнення ділення на нуль
#define MIN_SPEED_FOR_PERS_COUNT_KMH              10.0f   // Мінімальна швидкість для накопичення PERS-статистики
#define MIN_PERS_DISPLAY_DISTANCE_KM              0.1f    // Мінімальна відстань для показу PERS
#define RESET_PERSISTENT_TRIP_DISTANCE_KM         5000.0f // Автоскидання PERS після цієї відстані (км)

// --- Режим "Рух накатом" ---
#define COASTING_MIN_SPEED_KMH                    5.0f    // Мінімальна швидкість для визначення накату
#define COASTING_MIN_RPM                          1000.0f // Мінімальні оберти для визначення накату
#define COASTING_FUEL_THRESHOLD_LH                0.01f   // Поріг витрати (Л/год) нижче якого вважається накат

// =====================================================================================================================
// 8. КОЕФІЦІЄНТИ КОНВЕРСІЇ ОДИНИЦЬ
// =====================================================================================================================
#define SEC_IN_MINUTE                             60                                                                     // Секунд в хвилині
#define MINUTES_IN_HOUR                           60                                                                     // Хвилин в годині
#define SECONDS_IN_HOUR                           3600                                                                   // Секунд в годині
#define MILLISECONDS_IN_SECOND                    1000                                                                   // Мілісекунд в секунді
#define MICROSECONDS_IN_SECOND                    1000000UL                                                              // Мікросекунд в секунді
#define MILLILITERS_IN_LITER                      1000                                                                   // Мілілітрів в літрі
#define ML_PER_MIN_TO_L_PER_US_DENOMINATOR        ((float)MILLILITERS_IN_LITER * SEC_IN_MINUTE * MICROSECONDS_IN_SECOND) // (мл/хв) → (л/мкс)

// =====================================================================================================================
// 9. КОНФІГУРАЦІЯ ЕЛЕМЕНТІВ ВІДОБРАЖЕННЯ (РЕЖИМ 0 — ВИТРАТА ПАЛИВА)
// =====================================================================================================================
// --- Головне значення ---
#define MAIN_VAL_BUFFER_SIZE                      3        // Розмір буфера для згладжування основного значення (витрати/витрати на 100км)
#define MAIN_VALUE_FONT_SIZE                      4        // Розмір шрифту для головного числа
#define MAIN_VALUE_Y_POS                          4        // Y-координата головного числа
#define MAIN_VALUE_TEXT_X_OFFSET_ADJUST           11       // Додаткове зміщення по X для центрування
#define MAIN_VALUE_STATIONARY_TEXT                "-.--"   // Текст при стоянці
#define MAIN_VALUE_ERROR_TEXT                     "EEEE"   // Текст при помилці
#define MAIN_VALUE_COASTING_TEXT                  "COAS"   // Текст при накаті
#define MAIN_VALUE_TOWING_TEXT                    "TOWI"   // Текст при буксируванні
#define MAX_DISPLAY_L100KM_VALUE                  99.9f    // Максимальна витрата, що показується

// --- Одиниці виміру ---
#define MAIN_UNIT_FONT_SIZE_X                     1              // Масштаб X для шрифту одиниць
#define MAIN_UNIT_FONT_SIZE_Y                     1              // Масштаб Y для шрифту одиниць
#define MAIN_UNIT_Y_OFFSET                        12             // Відступ одиниць по Y від головного числа
#define MAIN_UNIT_RIGHT_ALIGN_OFFSET_PX           10             // Відступ для вирівнювання по правому краю
#define MAIN_UNIT_TEXT_FONT                       &MAIN_UKR_FONT // Шрифт одиниць

// --- Статистика TRIP/PERS ---
#define TRIP_STAT_Y_POS                           84            // Y-координата TRIP
#define PERS_STAT_Y_POS                           104           // Y-координата PERS
#define STAT_TEXT_PERS_X_POS                      3             // X-координата PERS
#define STAT_TEXT_TRIP_X_POS                      11            // X-координата TRIP
#define STAT_FONT_SIZE_X                          1             // Масштаб X шрифту статистики
#define STAT_FONT_SIZE_Y                          1             // Масштаб Y шрифту статистики
#define STAT_TEXT_FONT                            &MAIN_EN_FONT // Шрифт статистики

// --- Формат даних TRIP ---
#define TRIP_FUEL_DISPLAY_WIDTH                   4        // Ширина поля палива TRIP (символів)
#define TRIP_FUEL_DEFAULT_TEXT                    "--.-"   // Текст за замовчуванням
#define TRIP_FUEL_MIN_DISPLAY_L                   0.05f    // Мінімум палива для показу

#define TRIP_DISTANCE_DISPLAY_WIDTH               3        // Ширина поля відстані TRIP (символів)
#define TRIP_DISTANCE_DEFAULT_TEXT                "---"    // Текст за замовчуванням
#define TRIP_DISTANCE_MIN_DISPLAY_KM              0.1f     // Мінімум відстані для показу

#define PERS_L100KM_DISPLAY_WIDTH                 4        // Ширина поля середньої витрати (символів)
#define PERS_L100KM_DEFAULT_TEXT                  "--.-"   // Текст за замовчуванням

// =====================================================================================================================
// 10. КОНФІГУРАЦІЯ ЕЛЕМЕНТІВ ВІДОБРАЖЕННЯ (РЕЖИМ 1 — ШВИДКІСТЬ/ОБЕРТИ)
// =====================================================================================================================
// --- Тексти за замовчуванням ---
#define SP_SCR_DEFAULT_TEXT_KMH                   "---"    // Немає даних швидкості
#define SP_SCR_DEFAULT_TEXT_V                     "--.-"   // Немає даних напруги
#define SP_SCR_DEFAULT_TEXT_MS                    "--.--"  // Немає даних впорскування
#define SP_SCR_DEFAULT_TEXT_RPM                   "----"   // Немає даних RPM
#define SP_SCR_FUEL_DEFAULT_TEXT                  "--"     // Немає даних палива

// --- Напруга ---
#define SP_SCR_VOLTAGE_Y                          3        // Y-координата напруги
#define SP_SCR_VOLTAGE_X                          48       // X-координата напруги
#define SP_SCR_VOLTAGE_FONT_SIZE                  2        // Розмір шрифту напруги
#define SP_SCR_VOLTAGE_MIN_DISPLAY_V              0.5f     // Нижче цієї напруги показується "--.-"

// --- Швидкість ---
#define SP_SCR_SPEED_Y                            23       // Y-координата швидкості
#define SP_SCR_SPEED_X                            36       // X-координата швидкості
#define SP_SCR_SPEED_FONT_SIZE                    2        // Розмір шрифту швидкості

// --- Оберти ---
#define SP_SCR_RPM_Y                              43       // Y-координата RPM
#define SP_SCR_RPM_X                              24       // X-координата RPM
#define SP_SCR_RPM_FONT_SIZE                      2        // Розмір шрифту RPM
#define SP_SCR_RPM_SMOOTH_FACTOR                  0.8f     // Коефіцієнт згладжування RPM (0.0-1.0)

// --- Впорскування ---
#define SP_SCR_INJ_Y                              63       // Y-координата впорскування
#define SP_SCR_INJ_X                              24       // X-координата впорскування
#define SP_SCR_INJ_FONT_SIZE                      2        // Розмір шрифту впорскування
#define SP_SCR_INJ_FONT_SIZE_Y_FACTOR             2        // Фактор Y для шрифту впорскування
#define SP_SCR_INJ_SMOOTH_FACTOR                  0.8f     // Коефіцієнт згладжування впорскування

// --- Паливо ---
#define SP_SCR_FUEL_Y                             83       // Y-координата палива
#define SP_SCR_FUEL_X                             72       // X-координата палива
#define SP_SCR_FUEL_FONT_SIZE                     2        // Розмір шрифту палива
#define SP_SCR_FUEL_MIN_DISPLAY_PERCENT           0.9f     // Нижче цього % показується "--"

// =====================================================================================================================
// 11. КОНФІГУРАЦІЯ ЕЛЕМЕНТІВ ВІДОБРАЖЕННЯ (РЕЖИМ 2 — K-LINE DTC)
// =====================================================================================================================
// --- Статусні тексти ---
#define DTC_TEXT_NO_ERRORS                        "NO ERROR"   // Помилок немає
#define DTC_TEXT_CONNECTING                       "CONNECT"    // Триває підключення
#define DTC_TEXT_SLEEP                            "ACC OFF"    // ACC вимкнено
#define DTC_TEXT_ERROR                            "ERROR"      // Помилка зв'язку
#define DTC_TEXT_CLEANING                         "CLEANING"   // Виконується очищення

// --- Позиції та розміри статусних текстів ---
#define DTC_NO_ERRORS_X_POS                       10       // X "NO ERROR"
#define DTC_NO_ERRORS_Y_POS                       50       // Y "NO ERROR"
#define DTC_NO_ERRORS_FONT_SIZE_X                 1        // Масштаб X "NO ERROR"
#define DTC_NO_ERRORS_FONT_SIZE_Y                 1        // Масштаб Y "NO ERROR"

#define DTC_CONNECTING_X_POS                      10       // X "CONNECT"
#define DTC_CONNECTING_Y_POS                      50       // Y "CONNECT"
#define DTC_CONNECTING_FONT_SIZE_X                1        // Масштаб X "CONNECT"
#define DTC_CONNECTING_FONT_SIZE_Y                1        // Масштаб Y "CONNECT"

#define DTC_SLEEP_X_POS                           10       // X "SLEEP"
#define DTC_SLEEP_Y_POS                           50       // Y "SLEEP"
#define DTC_SLEEP_FONT_SIZE_X                     1        // Масштаб X "SLEEP"
#define DTC_SLEEP_FONT_SIZE_Y                     1        // Масштаб Y "SLEEP"

#define DTC_ERROR_X_POS                           10       // X "ERROR"
#define DTC_ERROR_Y_POS                           50       // Y "ERROR"
#define DTC_ERROR_FONT_SIZE_X                     1        // Масштаб X "ERROR"
#define DTC_ERROR_FONT_SIZE_Y                     1        // Масштаб Y "ERROR"

#define DTC_CLEANING_X_POS                        20       // X "CLEANING"
#define DTC_CLEANING_Y_POS                        50       // Y "CLEANING"
#define DTC_CLEANING_FONT_SIZE_X                  1        // Масштаб X "CLEANING"
#define DTC_CLEANING_FONT_SIZE_Y                  1        // Масштаб Y "CLEANING"
#define DTC_CLEANING_DISPLAY_TIME_MS              2000UL   // Час (мс) показу "CLEANING"

// --- Позиції та розміри коди помилок ---
#define DTC_CODE_FONT_SIZE_X                      1        // Масштаб X кодів
#define DTC_CODE_FONT_SIZE_Y                      1        // Масштаб Y кодів
#define DTC_CODE_X_POS                            20       // X-координата початку списку кодів
#define DTC_CODE_Y_START_POS                      20       // Y-координата першого коду
#define DTC_CODE_Y_STEP                           10       // Крок по Y між кодами (пікселів)
#define DTC_MAX_CODES_TO_DISPLAY                  5        // Максимум кодів на екрані

// --- Шрифти ---
#define DTC_STATUS_FONT                           &MAIN_EN_FONT  // Шрифт статусних текстів
#define DTC_CODE_TEXT_FONT                        &MAIN_EN1_FONT // Шрифт кодів помилок

// =====================================================================================================================
// 12. КОНФІГУРАЦІЯ СЕРВІСНОГО РЕЖИМУ
// =====================================================================================================================
// --- Відображення ---
#define MAX_SERVICE_DISPLAY_LINES                 5        // Кількість рядків даних

// --- Кастомні підписи рядків ---
#define SERVICE_LINE_1_CUSTOM_TEXT                " ECT"   // Engine Coolant Temperature
#define SERVICE_LINE_2_CUSTOM_TEXT                " CO2"   // Lambda / CO2 корекція
#define SERVICE_LINE_3_CUSTOM_TEXT                " IAT"   // Intake Air Temperature
#define SERVICE_LINE_4_CUSTOM_TEXT                " ETC"   // Electronic Throttle Control (кут дроселя)
#define SERVICE_LINE_5_CUSTOM_TEXT                " TDC"   // Top Dead Center (кут запалення)

// --- Таймінги ---
#define SERVICE_MODE_KLINE_CONNECT_TIMEOUT_MS     10000    // Таймаут підключення K-Line (мс)
#define SERVICE_MODE_KLINE_READ_INTERVAL_MS       1000     // Інтервал читання даних (мс)

// --- Шрифти та координати ---
#define SERVICE_MODE_DISPLAY_FONT_SIZE            1        // Розмір шрифту значень
#define SERVICE_MODE_DISPLAY_TEXT_FONT_SIZE       1        // Розмір шрифту підписів
#define SERVICE_MODE_DISPLAY_Y_START_POS          5        // Y-координата початку списку
#define SERVICE_MODE_DISPLAY_LINE_HEIGHT          20       // Висота рядка

// --- Шрифт ---
#define SERVICE_LINE_TEXT_FONT                    &MAIN_EN1_FONT // Шрифт даних

// --- Тексти статусу K-Line ---
#define SERVICE_KLINE_CONNECTED_TEXT              "CONNECT"      // K-Line підключено
#define SERVICE_KLINE_DISABLED_TEXT               "DISABLED"     // K-Line вимкнено
#define SERVICE_KLINE_TEXT_FONT                   &MAIN_EN_FONT  // Шрифт статусу

// --- Позиції "CONNECT" ---
#define SERVICE_KLINE_CONNECTED_X_POS             4        // X "CONNECT"
#define SERVICE_KLINE_CONNECTED_Y_POS             60       // Y "CONNECT"
#define SERVICE_KLINE_CONNECTED_FONT_SIZE_X       1        // Масштаб X "CONNECT"
#define SERVICE_KLINE_CONNECTED_FONT_SIZE_Y       1        // Масштаб Y "CONNECT"

// --- Позиції "DISABLED" ---
#define SERVICE_KLINE_DISABLED_X_POS              5        // X "DISABLED"
#define SERVICE_KLINE_DISABLED_Y_POS              55       // Y "DISABLED"
#define SERVICE_KLINE_DISABLED_FONT_SIZE_X        1        // Масштаб X "DISABLED"
#define SERVICE_KLINE_DISABLED_FONT_SIZE_Y        1        // Масштаб Y "DISABLED"

// --- Мапування груп K-Line ---
const uint8_t SERVICE_MEASUREMENT_MAP[MAX_SERVICE_DISPLAY_LINES][2] = {
    {1, 2}, // Група 1, параметр 2: температура ОЖ
    {1, 3}, // Група 1, параметр 3: лямбда-корекція
    {2, 4}, // Група 2, параметр 4: температура повітря на впуску
    {3, 2}, // Група 3, параметр 2: кут дросельної заслінки
    {3, 3}  // Група 3, параметр 3: кут випередження запалення
};

// =====================================================================================================================
// 13. ТЕКСТИ ПОМИЛОК ДЛЯ РЕЖИМІВ БЕЗ ДАНИХ
// =====================================================================================================================
// --- Тексти статусу ---
#define MODE_ERROR_TEXT_MODE_0                     "NO DATA0"        // Немає даних для режиму 0
#define MODE_ERROR_TEXT_MODE_1                     "NO DATA1"        // Немає даних для режиму 1
#define MODE_ERROR_TEXT_MODE_2                     "DISABLED"        // K-Line вимкнено
#define MODE_ERROR_TEXT_DEFAULT                    "NO DATA-"        // Немає даних (за замовчуванням)

// --- Координати ---
#define MODE_ERROR_TEXT_X_POS                      5                 // X-координата тексту помилки
#define MODE_ERROR_TEXT_Y_POS                      55                // Y-координата тексту помилки
#define MODE_ERROR_TEXT_SIZE                       1                 // Розмір шрифту помилки

// --- Шрифт ---
#define MODE_ERROR_TEXT_FONT                       &MAIN_EN1_FONT    // Шрифт помилок

// =====================================================================================================================
// 14. КОНСТАНТИ EEPROM
// =====================================================================================================================
#define EEPROM_SIZE                               512             // Доступний розмір EEPROM (байт)
#define EEPROM_ADDR_MAGIC                         0               // Адреса магічного числа
#define EEPROM_ADDR_TRIP_FUEL                     4               // Адреса TRIP Fuel (float)
#define EEPROM_ADDR_TRIP_DISTANCE                 8               // Адреса TRIP Distance (float)
#define EEPROM_ADDR_PERS_FUEL                     12              // Адреса Persistent Fuel (float)
#define EEPROM_ADDR_PERS_DISTANCE                 16              // Адреса Persistent Distance (float)
#define EEPROM_ADDR_DISPLAY_MODE                  20              // Адреса поточного режиму дисплея (int)
#define EEPROM_MAGIC_NUMBER                       0xDEADC0DEUL    // Магічне число для перевірки цілісності

// =====================================================================================================================
// 15. КОНФІГУРАЦІЯ K-LINE (KWP1281)
// =====================================================================================================================
#define KLINE_CONNECT_TO_MODULE                       0x01       // Адреса ЕБУ (0x01 = двигун)
#define KLINE_MODULE_BAUD_RATE                        4800       // Швидкість UART після 5-бод ініціалізації
#define KLINE_IS_FULL_DUPLEX                          true       // Чи є ехо на лінії
#define KLINE_DTC_BUFFER_MAX_FAULT_CODES              16         // Максимальна кількість кодів DTC у буфері
#define KLINE_MAX_CONNECTION_RETRIES                  3          // Максимальна кількість спроб підключення
#define KLINE_RECONNECT_DELAY_MS                      5000UL     // Затримка (мс) між спробами перепідключення
#define KLINE_UPDATE_INTERVAL_MS                      800UL      // Інтервал (мс) оновлення DTC
#define KLINE_BLINK_ON_DURATION_MS                    500UL      // Тривалість ON фази блимання при KLINE CONNECTING
#define KLINE_BLINK_OFF_DURATION_MS                   500UL      // Тривалість OFF фази блимання при KLINE CONNECTING
#define KLINE_MIN_RPM_FOR_ENGINE_RUNNING              100.0f     // Мінімальні оберти для визначення "двигун працює"
#define KLINE_SERIAL                                  Serial2    // UART для K-Line
#define KLINE_SERIAL_CONFIG                           SERIAL_8N1 // Конфігурація: 8N1
#define KLINE_CONNECTED_TASK_IDLE_DELAY_MS            250        // Затримка (мс) у стані KLINE CONNECTED
#define KLINE_CONNECT_TASK_RETRY_DELAY_MS             2000       // Затримка (мс) між спробами підключення
#define KLINE_MAX_CONNECTION_TIMEOUT_MS               8000UL     // Максимальний час (мс) очікування відповіді ЕБУ
#define KLINE_PERM_DISCONNECTED_BLINK_ON_DURATION_MS  1500       // ON фаза при KLINE PERMANENTLY DISCONNECTED
#define KLINE_PERM_DISCONNECTED_BLINK_OFF_DURATION_MS 500        // OFF фаза при KLINE PERMANENTLY DISCONNECTED

// =====================================================================================================================
// 16. НАЛАШТУВАННЯ ШРИФТІВ
// =====================================================================================================================
#include "Fonts/all_fonts.h"         // Підключення всіх кастомних шрифтів
#define MAIN_UKR_FONT verdanabUkr6   // Український шрифт (одиниці виміру)
#define MAIN_EN_FONT FreeSans10pt8b  // Англійський шрифт 10pt (статистика, статуси)
#define MAIN_EN1_FONT FreeSans9pt7b  // Англійський шрифт 9pt (коди помилок, сервісний режим)

// =====================================================================================================================
// 17. КОНФІГУРАЦІЯ FREERTOS
// =====================================================================================================================
#define __FREERTOS                                1        // Увімкнути FreeRTOS (1 = так, 0 = ні)

#endif // CONFIG_H
