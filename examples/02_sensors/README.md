# EmbUI Example

### Dynamic Sensors display/update template

Шаблон проекта демонстрирующий способы отображения информации с различных сенсоров

  - Формирование "дисплеев" на странице
    - дисплей представляет из себя div с текстовым содержимым на основе специального CSS стиля
    - Стили можно использовать как свои так и встроенный в фреймворк стиль по-умолчанию
    - обновление данных на дисплее производится через периодическую отсылку пар id:value, данные обновляются на "дисплее" с соответсвующим id

<img src="display.png" alt="display example" width="50%"/>

Установка:

 - в папку data развернуть файлы фреймворка из архива /resources/data.zip **БЕЗ** перезаписи содержимого
 - удалить файл index.html.gz
 - собрать проект в platformio
 - залить FS в контроллер `pio run -t uploadfs`
 - залить прошивку в контроллер `pio run -t upload`