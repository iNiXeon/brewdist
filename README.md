# BrewDist Monitor (2026)
Система дистанционного мониторинга и управления процессом дистилляции и затирания на базе **ESP32**. 
Проект позволяет отслеживать температуру в кубе (T1) и на узле отбора (T2), управлять паузами затирания и получать уведомления в Telegram через Cloudflare Workers. Возможно добавление упправления ТЭНом и клапаном отбора.

---
### Поддержать (TON): UQCB4c5bfPCEyEc0H5RnqMjyHRagN9jybugF_vV614oPgZbs
---

## 🛠 Аппаратная часть (Hardware)

### Используемые компоненты:
* **Контроллер:** ESP32 (рекомендуется DevKit V1).
* **Дисплей:** OLED 128x64 (драйвер SSD1306, подключение I2C).
* **Датчики температуры:** Цифровые DS18B20 (2 шт.) на одной шине.
* **Питание:** Стабильные 5V (минимум 1A).

### Схема подключения (Pins):
По умолчанию в коде используются следующие пины:
* **DS18B20 (Шина OneWire):** Пин `GPIO 4`. Не забудьте подтягивающий резистор 4.7кОм между VCC и Данными.
* **Дисплей OLED (I2C):** 
  * `SCL` -> Пин `GPIO 6`
  * `SDA` -> Пин `GPIO 5`
* **Питание датчиков:** 3.3V или 5V (зависит от модели).

> **Как изменить пины?**  
> В начале файла `main.cpp` найдите строку `#define SENSOR_PIN 4` для датчиков и строку инициализации дисплея `u8x8(6, 5, ...)` для изменения I2C пинов.

---

## ☁️ Настройка Cloudflare Worker

Для безопасной работы с Telegram API без прямого открытия портов используется прокси-воркер.

1. Создайте аккаунт на [Cloudflare](https://dash.cloudflare.com/).
2. Перейдите в **Build -> Compute -> Workers & Pages -> Create application**.
3. Назовите его (например, `brew-tg-proxy`).
4. Вставьте следующий код в редактор воркера:

```javascript
export default {
  async fetch(request, env) {
    const url = new URL(request.url);
    
    // 1. Проверка пути: работаем только с запросами к API Бота
    if (!url.pathname.startsWith('/bot')) {
      return new Response('Not Found', { status: 404 });
    }

    // 2. Формируем целевой URL для Telegram
    // Включаем url.search, чтобы поддерживать параметры и в строке (GET/URL params)
    const targetUrl = `https://api.telegram.org${url.pathname}${url.search}`;

    // 3. Подготовка тела запроса (КРИТИЧЕСКИЙ МОМЕНТ)
    // Читаем тело как Buffer, чтобы гарантированно передать его целиком
    let bodyContent = null;
    if (request.method === 'POST' || request.method === 'PUT') {
      try {
        bodyContent = await request.arrayBuffer();
      } catch (e) {
        console.error("Ошибка чтения Body:", e.message);
      }
    }

    // 4. Создаем новый запрос к официальному API
    const modifiedRequest = new Request(targetUrl, {
      method: request.method,
      headers: {
        // Пробрасываем важные заголовки, если они есть
        'Content-Type': request.headers.get('Content-Type') || 'application/json',
        'Accept': '*/*',
        'User-Agent': 'Yagzhin-Brew-Monitor/1.1 (ESP32)',
      },
      body: bodyContent,
      redirect: 'follow'
    });

    // 5. Выполняем запрос и возвращаем результат
    try {
      const response = await fetch(modifiedRequest);
      
      // Клонируем ответ, чтобы иметь возможность добавить CORS заголовки (если нужно будет для веб-морды)
      const newResponse = new Response(response.body, response);
      newResponse.headers.set('Access-Control-Allow-Origin', '*');
      
      return newResponse;
    } catch (e) {
      return new Response('Worker Proxy Error: ' + e.message, { 
        status: 500,
        headers: { 'Content-Type': 'text/plain; charset=utf-8' }
      });
    }
  }
};
```

После публикации вы получите адрес вида brew-tg-proxy.ваш-ник.workers.dev. Его нужно будет ввести в настройках WiFi контроллера.

## Настройка Telegram
<br> Найдите @BotFather в Telegram.  
<br> Создайте нового бота (/newbot) и получите API Token.  
<br> Узнайте свой Chat ID (с помощью бота @userinfobot).  
<br> Эти данные понадобятся при первом запуске устройства.  

## Первый запуск и конфигурация
<br> При первом включении (или если WiFi не найден), ESP32 перейдет в режим точки доступа.
<br> Подключитесь к сети WiFi с названием "Brew_CTRL" со смартфона.
<br> Откроется страница настройки (Captive Portal). Если не открылась — перейдите по адресу 192.168.4.1.
<br> Введите данные:  
<br> SSID и Password вашей домашней сети.  
<br> Worker URL: Ваш адрес Cloudflare (без https://).  
<br> Telegram Token: Токен вашего бота.  
<br> Chat ID: Ваш ID.  
<br> UTC Offset: Смещение часового пояса (например, 5 для Екатеринбурга).  
<br> Нажмите Save. Контроллер перезагрузится и пришлет в Telegram сообщение: ✅ BrewDist Monitor Online.

## Данное программное обеспечение предоставляется на условиях Custom Non-Commercial License:

<br> Личное использование: Бесплатно и без ограничений.
<br> Копирайт: При использовании кода в своих открытых проектах обязательна ссылка на автора и сохранение заголовков в файлах.
<br> Коммерческое использование: ЗАПРЕЩЕНО без письменного согласия автора. Это включает продажу готовых устройств с данным кодом или использование кода для оказания платных услуг.
<br> Для получения коммерческой лицензии свяжитесь со мной: [mixeon@gmail.com]

## 📄 License & Usage

This project is licensed under a **Custom Non-Commercial License**. 

You are entirely free to:
* Download, compile, and use the code for your personal brewing/distilling setups.
* Modify the code to fit your specific hardware.
* Share your modifications, provided you keep the original copyright notice.

You are **NOT** allowed to:
* Sell devices running this software.
* Use this software to produce goods for commercial sale.
* Re-distribute the code as part of a commercial product.

**💼 Commercial Licensing:**
If you want to use this code in a commercial product, you must obtain a commercial license. Please contact me at: `mixeon@gmail.com`