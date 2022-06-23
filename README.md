# ProcessAnalyzer
 
Проект разрабатывается совместно с <a href="https://github.com/Milikovv18">Milikovv18</a>.
Цель проекта - написание приложения, позволяющего пользователю получать информацию о загружаемых процессами библиотеках DLL и заменять функции из этих библиотек (hooking). Для замены необходимо описать функции, используемые для замены, и собрать библиотеку DLL, которая будет их экспортировать.

На данный момент реализован не весь функционал.
Список ограничений:
- Отсутствует взаимодействие с процессами разрядности 32
- Невозможно обратить хукинг (вернуть функцию в исходное состояние)
- Не выгружаются библиотеки с функциями-заменами
- Функция-трамплин - единственная, заменить можно только одну целевую функцию

<b>Внимание! Не рекомендуется использовать пункт контекстного меню "Unhook" - это может привести к ошибочному завершению программы.</b>

Демонстрация работы программы:

https://user-images.githubusercontent.com/55055449/175368939-d37e9666-f379-4499-95d1-e65e654f2f82.mp4

<b>Сборка:</b>

ProcessAnalyzer - Qt 6.1.3 MSVC 2019 profile

ProcessAnalyzerDll, Test - MS Visual Studio 2019   (DLL)
