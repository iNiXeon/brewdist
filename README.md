# BrewDist Monitor (2026) 🍺⚒️
Система дистанционного мониторинга и управления процессом дистилляции и затирания на базе **ESP32-C3**.

Проект позволяет отслеживать температуру в кубе (T1) и на царге/узле отбора (T2), управлять паузами затирания и получать уведомления в Telegram через Cloudflare Workers. Реализовано управление ТЭНом и звуковая индикация.

---
### Поддержать проект (TON): `UQCB4c5bfPCEyEc0H5RnqMjyHRagN9jybugF_vV614oPgZbs`
---

## 🛠 Аппаратная часть (Hardware)

### Используемые компоненты:
* **Контроллер:** ESP32-C3 (совместимо с другими ESP32 при переназначении пинов).
* **Дисплей:** OLED 128x64 (I2C, библиотека GyverOLED).
* **Датчики температуры:** DS18B20 (2 шт.) на одной шине OneWire.
* **Управление:** 4 тактовые кнопки (Up, Down, Ok, Back) в составе дисплея.

### Схема подключения (Pins по умолчанию):
В соответствии с актуальной прошивкой используются следующие пины:
* **DS18B20 (OneWire):** Пин `GPIO 10` (требуется резистор 4.7кОм к 3.3V).
* **Дисплей OLED (I2C):** 
  * `SCL` -> Пин `GPIO 9`
  * `SDA` -> Пин `GPIO 8`
* **Исполнительные устройства:**
  * `Реле/ТЭН` -> Пин `GPIO 2`
  * `Buzzer (Пищалка)` -> Пин `GPIO 1`
* **Кнопки:** `GPIO 6` (Up), `5` (Down), `4` (Ok), `3` (Back).

> **💡 Как изменить пины?** > Все настройки портов находятся в верхней части файла `main.cpp`. Для кнопок используются дефайны в начале файла.

---

## ☁️ Настройка Cloudflare Worker (Telegram Proxy)

Для стабильной работы с Telegram API без SSL-ошибок и блокировок используется прокси-воркер.

1. Создайте аккаунт на [Cloudflare](https://dash.cloudflare.com/).
2. Перейдите в **Workers & Pages -> Create application**.
3. Вставьте следующий код в редактор:

```javascript
export default {
  async fetch(request, env) {
    const url = new URL(request.url);
    if (!url.pathname.startsWith('/bot')) return new Response('Not Found', { status: 404 });

    const targetUrl = `https://api.telegram.org${url.pathname}${url.search}`;
    let bodyContent = null;
    if (request.method === 'POST') bodyContent = await request.arrayBuffer();

    const modifiedRequest = new Request(targetUrl, {
      method: request.method,
      headers: {
        'Content-Type': request.headers.get('Content-Type') || 'application/json',
        'User-Agent': 'Yagzhin-Brew-Monitor/1.1 (ESP32)',
      },
      body: bodyContent
    });

    try {
      const response = await fetch(modifiedRequest);
      const newResponse = new Response(response.body, response);
      newResponse.headers.set('Access-Control-Allow-Origin', '*');
      return newResponse;
    } catch (e) {
      return new Response('Proxy Error: ' + e.message, { status: 500 });
    }
  }
};

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

## 📄 License & Usage
Данное программное обеспечение предоставляется на условиях Custom Non-Commercial License:

<br> Личное использование: Бесплатно и без ограничений.
<br> Копирайт: При использовании кода в своих открытых проектах обязательна ссылка на автора и сохранение заголовков в файлах.
<br> Коммерческое использование: ЗАПРЕЩЕНО без письменного согласия автора. Это включает продажу готовых устройств с данным кодом или использование кода для оказания платных услуг.
<br> Для получения коммерческой лицензии свяжитесь со мной: [mixeon@gmail.com]


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