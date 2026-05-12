/*
 * ============================================================
 *  TODO-лист
 * ============================================================
 * ДОБАВИТЬ ЗВУКИ ПРИ ШАГАНИИ И ШОРКАНЬЕ СНЕГА НА ЛОКАЦИИ СНЕЖНОГО ЛЕСА (И В ЦЕЛОМ IF SNOW == SHORKSOUND)
 * ДОБАВИТЬ СЛЕД ЛОКАЦИЮ - РУИНЫ/СНЕЖГРАД
 * ДОБАВИТЬ БОЕВКУ И ФАЙТЫ В ЦЕЛОМ
 * ДОБАВИТЬ БОЛЬШЕ ВРАГОВ, НПС И ВЗАИМОДЕЙСТВИЙ
 * РАСКИДАТЬ ПО РАЗНЫМ ФАЙЛАМ КОД
 * ДОБАВИТЬ ФЛАУИ И БОЕВКУ С СИСТЕМОЙ ПОЩАДИТЬ/АТАКОВАТЬ
 * ДОБАВИТЬ ИКОНКУ СОХРАНЕНИЯ
 * ДОБАВИТЬ ВТОРОГО ПЕРСА НА ВТОРУЮ ЛОКАЦИЮ
 * ПРОТЕСТИТЬ ЗАПУСК НА ДРУГИХ КОМПАХ
 * СУНДУК НА ВТОРОЙ ЛОКАЦИИ И ВЗАИМОДЕЙСТВИЕ С НИМ
 * ПОДКЛЮЧИТЬ ГИТ
 * ============================================================
 */

#include <SFML/Audio.hpp>    // Звуки и музыка
#include <SFML/Graphics.hpp> // Графика, окно, спрайты, текст
#include <iostream>         // Ввод/вывод в консоль (для отладки)
#include <vector>           // Контейнер для динамических массивов (препятствия)

 // ---------- Перечисление состояний игры ----------
enum class GameState { MENU, PLAYING }; // Игра либо в меню, либо в процессе

// ---------- Перечисление игровых локаций ----------
enum class GameLocation { FIRST_LOCATION, SNOW_FOREST };

// ---------- Структура для удобной работы с кнопками меню ----------
struct Button {
    sf::RectangleShape shape;   // Прямоугольник кнопки
    sf::Text text;              // Текст внутри кнопки

    // Проверка, находится ли точка внутри кнопки
    bool contains(sf::Vector2f point) const {
        return shape.getGlobalBounds().contains(point);
    }

    // Установка цвета заливки прямоугольника
    void setFillColor(const sf::Color& color) {
        shape.setFillColor(color);
    }

    // НОВЫЙ МЕТОД: меняет цвет текста кнопки
    void setTextColor(const sf::Color& color) {
        text.setFillColor(color);
    }
};

/*
 * class NPC { // Сделать потом (заготовка для будущего класса NPC)
 * ;
 */

 // ---------- Проверка столкновения спрайта с вектором прямоугольных препятствий ----------
bool checkCollision(const sf::Sprite& sprite, const std::vector<sf::RectangleShape>& obstacles) {
    // Получаем глобальные границы спрайта один раз
    sf::FloatRect spriteBounds = sprite.getGlobalBounds();
    // Проходим по ВСЕМ препятствиям в переданном векторе
    for (const auto& obstacle : obstacles) {
        // Если пересекается хотя бы с одним — возвращаем true
        if (spriteBounds.intersects(obstacle.getGlobalBounds())) {
            return true; // Коллизия обнаружена!
        }
    }
    return false; // Коллизий нет
}

// ---------- Шаблонная функция для проверки пересечения двух объектов (используется для NPC и игрока) ----------
template<typename T1, typename T2>
bool NPCcheckCollision(const T1& a, const T2& b) {
    // Сравнивает глобальные границы двух объектов
    return a.getGlobalBounds().intersects(b.getGlobalBounds());
}

// ---------- Режим позиционирования карты ----------
enum class CenteringMode {
    CENTER,      // По центру экрана
    TOP_LEFT,    // В левом верхнем углу
    TOP_CENTER,  // По центру сверху
    BOTTOM_LEFT  // В левом нижнем углу
};

// ---------- Функция для позиционирования и масштабирования карты на экране ----------
sf::FloatRect centringMapAdvanced(sf::Texture& texture, sf::Sprite& map, float mapScale,
    const sf::Vector2u& windowSize, CenteringMode mode = CenteringMode::CENTER) {
    // Устанавливаем масштаб спрайта
    map.setScale(mapScale, mapScale);

    // Вычисляем физические размеры текстуры с учётом масштаба
    float textureWidth = texture.getSize().x * mapScale;
    float textureHeight = texture.getSize().y * mapScale;
    float posX = 0.0f, posY = 0.0f;

    // В зависимости от выбранного режима рассчитываем координаты
    switch (mode) {
    case CenteringMode::CENTER:
        posX = (windowSize.x - textureWidth) / 2.f;
        posY = (windowSize.y - textureHeight) / 2.f;
        break;
    case CenteringMode::TOP_LEFT:
        posX = 0;
        posY = 0;
        break;
    case CenteringMode::TOP_CENTER:
        posX = (windowSize.x - textureWidth) / 2.f;
        posY = 0;
        break;
    case CenteringMode::BOTTOM_LEFT:
        posX = 0;
        posY = windowSize.y - textureHeight;
        break;
    }

    // Применяем рассчитанную позицию к спрайту карты
    map.setPosition(posX, posY);

    // Возвращаем границы, которые занимает карта (важно для последующих проверок)
    return sf::FloatRect(posX, posY, textureWidth, textureHeight);
}

// ==================================================================
// ГЛАВНАЯ ФУНКЦИЯ
// ==================================================================
int main() {
    // ==================================================================
    // ИНИЦИАЛИЗАЦИЯ ОКНА
    // ==================================================================
    sf::ContextSettings settings;                // Настройки OpenGL-контекста
    settings.antialiasingLevel = 8;             // Включаем сглаживание уровня 8x
    // Создаём окно на весь экран с заданными настройками
    sf::RenderWindow window(sf::VideoMode::getDesktopMode(), "SanserTale",
        sf::Style::Fullscreen, settings);
    window.setFramerateLimit(60);               // Ограничение FPS до 60 кадров/с
    sf::Vector2u winSize = window.getSize();    // Запоминаем размеры окна (рабочего стола)

    // Устанавливаем начальную локацию, а также флаги для отслеживания перехода
    GameLocation currentLocation = GameLocation::FIRST_LOCATION;
    bool firstForestLoad = true;                // Флаг, что лес загружается впервые (для будущей логики)
    GameLocation previousLocation = currentLocation; // Для определения смены локации

    // Начальное состояние — ГЛАВНОЕ МЕНЮ
    GameState currentState = GameState::MENU;
    int selectedButton = 0; // 0 = "Start Game", 1 = "Exit" (текущая выбранная кнопка)

    // ==================================================================
    // ЗАГРУЗКА РЕСУРСОВ
    // ==================================================================

    sf::Image icon;
    if (icon.loadFromFile("Resources/SANSERTALE_icon.png")) {
        window.setIcon(icon.getSize().x, icon.getSize().y, icon.getPixelsPtr());
    }

    // ---------- Шрифт ----------
    sf::Font PixelFont;
    if (!PixelFont.loadFromFile("Resources/Text/Akedopikuseru-Regular.otf")) {
        std::cerr << "ERROR: Font not found!" << std::endl;
        return 1; // При отсутствии шрифта завершаем программу с ошибкой
    }

    // ---------- Текстура первой локации ----------
    sf::Texture First_Location;
    if (!First_Location.loadFromFile("Resources/Textures/FIRST_LOCATION.png")) {
        std::cerr << "ERROR: Texture not found!" << std::endl;
        return 1;
    }
    sf::Sprite First_LocationSprite(First_Location); // Создаём спрайт на основе текстуры

    // Границы карты (временно используются для вывода заголовка)
    sf::FloatRect mapSize = First_LocationSprite.getGlobalBounds();

    // Рассчитываем и применяем позиционирование карты (центр экрана, масштаб x3)
    sf::FloatRect mapBounds = centringMapAdvanced(First_Location, First_LocationSprite, 3.0f,
        winSize, CenteringMode::CENTER);

    // ---------- Текстура снежного леса ----------
    sf::Texture snow_Forest;
    if (!snow_Forest.loadFromFile("Resources/Textures/SNOW_FOREST.png")) {
        std::cerr << "ERROR: Texture not found!" << std::endl;
        return 1;
    }
    sf::Sprite snow_ForestSprite(snow_Forest); // Спрайт леса

    // Лес позиционируется по нижнему левому углу, масштаб x5
    sf::FloatRect forestBounds = centringMapAdvanced(snow_Forest, snow_ForestSprite, 5.0f,
        winSize, CenteringMode::BOTTOM_LEFT);

    // ---------- Игровая камера (вид) ----------
    sf::View gameView;
    gameView.setSize(winSize.x, winSize.y);                // Размер области обзора = размер окна
    gameView.setCenter(winSize.x / 2.f, winSize.y / 2.f);  // Начальный центр камеры — середина экрана

    // ---------- Текст заголовка (отображается только в первой локации) ----------
    sf::Text gameTitle("SANSERTALE", PixelFont, 50);
    gameTitle.setPosition(mapSize.width / 2.f + 140, mapSize.height / 2.f + 535);

    // ---------- Текст диалога (появляется при взаимодействии с NPC) ----------
    sf::Text NPCTitle("Welcome to \nSANSERTALE!", PixelFont, 15);
    NPCTitle.setPosition(600, 310);
    bool dialogOpened = false; // Флаг: открыт ли сейчас диалог

    // ---------- Персонаж игрока (Санс) ----------
    sf::Texture SansTexture;
    if (!SansTexture.loadFromFile("Resources/Anim/SANS_WALK_ANIM.png")) {
        std::cerr << "ERROR: Animation not found!" << std::endl;
        return 1;
    }
    sf::Sprite SansSprite(SansTexture);
    // Размер одного кадра в спрайт-листе (анимация по строкам и столбцам)
    int frameWidth = 256;
    int frameHeight = 253;
    SansSprite.setOrigin({ frameWidth / 2.f, frameHeight / 2.f }); // Центр спрайта — в центр кадра
    // Начальная позиция в первой локации (примерно в центре-внизу)
    SansSprite.setPosition(mapSize.width / 2.f + 410, mapSize.height / 2.f + 835);
    SansSprite.setTextureRect({ {0, 0}, {frameWidth, frameHeight} }); // Показываем первый кадр
    SansSprite.setScale(0.8f, 0.8f); // Немного уменьшаем персонажа

    // Параметры анимации
    int currentFrame = 0;          // Текущий столбец (кадр)
    int currentRow = 0;           // Текущая строка (направление: 0=вниз, 1=вправо, 2=вверх, 3=влево)
    float animationSpeed = 0.09f; // Задержка между сменой кадров (в секундах)
    sf::Clock deltaClock;         // Часы для расчёта deltaTime (времени кадра)
    sf::Clock animClock;          // Часы для контроля частоты смены анимационных кадров

    // Скорость передвижения персонажа (базовая)
    float baseSpeed = 350.0f;
    // Множитель скорости, может меняться для разных локаций (например, в лесу медленнее)
    float speedMultiplier = 1.0f;

    // ---------- NPC (Фриск) ----------
    sf::Texture FriskTexture;
    if (!FriskTexture.loadFromFile("Resources/Models/FRISK.png")) {
        std::cerr << "ERROR: Frisk not found!" << std::endl;
        return 1;
    }
    sf::Sprite FriskSprite(FriskTexture);
    FriskSprite.setPosition(650, 350); // Положение на карте первой локации
    FriskSprite.setScale(0.67f, 0.67f);

    // ---------- Звуки и музыка ----------
    // Меню
    sf::Music menuMusic;
    if (!menuMusic.openFromFile("Resources/Audio/mus_menu0.ogg")) {
        std::cerr << "ERROR: Music not found!" << std::endl;
    }
    menuMusic.play();          // Запускаем музыку меню сразу
    menuMusic.setLoop(true);   // Зацикливаем

    // Тема Санса (первая локация)
    sf::Music sansTheme;
    if (!sansTheme.openFromFile("Resources/Audio/Sans_theme.mp3")) {
        std::cerr << "ERROR: Music not found!" << std::endl;
    }

    // Звук навигации по меню (короткий) – используется sf::Music, что не совсем корректно, но работает
    sf::Music firstMegalovania;
    if (!firstMegalovania.openFromFile("Resources/Audio/First_megalovania.mp3")) {
        std::cerr << "ERROR: Music not found!" << std::endl;
    }

    // Звук выбора пункта меню
    sf::Music selectSound;
    if (!selectSound.openFromFile("Resources/Audio/undertale-select-sound.mp3")) {
        std::cerr << "ERROR: Music not found!" << std::endl;
    }

    // Музыка снежного леса
    sf::Music snowMusic;
    if (!snowMusic.openFromFile("Resources/Audio/SNOWDIN.mp3")) {
        std::cout << "ERROR Snow music wasn't found" << std::endl;
    }

    sf::Music FriskTalk;
    if (!FriskTalk.openFromFile("Resources/Audio/Frisk.mp3")) {
        std::cout << "ERROR Frisk Talk wasn't found" << std::endl;
    }

    sf::Music texting;
    if (!texting.openFromFile("Resources/Audio/undertale-text.mp3")) {
        std::cout << "ERROR Texting wasn't found" << std::endl;
    }

    sf::Music snowShagi;
    if (!snowShagi.openFromFile("Resources/Audio/SHAGI.mp3")) {
        std::cout << "ERROR Shagi wasn't found" << std::endl;
    }
    

    // ==================================================================
    // СИСТЕМА ПРЕПЯТСТВИЙ ДЛЯ ПЕРВОЙ ЛОКАЦИИ
    // ==================================================================
    std::vector<sf::RectangleShape> obstacles; // Вектор для хранения препятствий

    // Удобная лямбда-функция для добавления прямоугольного препятствия
    auto addObstacle = [&obstacles](float x, float y, float w, float h, sf::Color color = sf::Color::Black) {
        sf::RectangleShape obs(sf::Vector2f(w, h));
        obs.setPosition(x, y);
        obs.setFillColor(color); // По умолчанию чёрный, но можно сделать прозрачным, чтобы не портить вид
        obstacles.push_back(obs);
        };

    // Конкретное препятствие: невидимые "врата" для перехода на лесную локацию
    addObstacle(830, 195, 243, 5, sf::Color::Transparent);

    // ==================================================================
    // СИСТЕМА ПРЕПЯТСТВИЙ ДЛЯ ЛЕСНОЙ ЛОКАЦИИ (границы и переходы)
    // ==================================================================
    std::vector<sf::RectangleShape> forestObstacles; // Препятствия внутри леса

    auto addForestObstacle = [&forestObstacles](float x, float y, float w, float h, sf::Color color = sf::Color::Black) {
        sf::RectangleShape obs(sf::Vector2f(w, h));
        obs.setPosition(x, y);
        obs.setFillColor(color);
        forestObstacles.push_back(obs);
        };

    // Вертикальные невидимые стены слева и справа карты леса, чтобы Санс не выходил за края
    addForestObstacle(180, -2106, 30, 3185, sf::Color::Transparent);
    addForestObstacle(1780, -2106, 30, 3185, sf::Color::Transparent);

    // Препятствия, возвращающие игрока из леса обратно в первую локацию
    std::vector<sf::RectangleShape> forestReturnObstacles;
    auto addForestReturnObstacle = [&forestReturnObstacles](float x, float y, float w, float h, sf::Color color = sf::Color::Black) {
        sf::RectangleShape obs(sf::Vector2f(w, h));
        obs.setPosition(x, y);
        obs.setFillColor(color);
        forestReturnObstacles.push_back(obs);
        };

    // Переходная зона: невидимый прямоугольник в определённом месте леса
    addForestReturnObstacle(forestBounds.width / 2.0f - 130, forestBounds.height / 2.0f - 525, 265, 10, sf::Color::Transparent);

    // ==================================================================
    // КНОПКИ ГЛАВНОГО МЕНЮ
    // ==================================================================
    Button startButton, exitButton;

    // Кнопка "Start Game"
    startButton.shape.setSize({ 400, 70 });
    startButton.shape.setPosition(winSize.x / 2.f - 195, winSize.y / 2.f - 40);
    startButton.text.setFont(PixelFont);
    startButton.text.setString("Start Game");
    startButton.text.setCharacterSize(30);
    startButton.text.setFillColor(sf::Color::White);
    startButton.setFillColor(sf::Color::Black);
    // Центрируем текст внутри кнопки
    sf::FloatRect stRect = startButton.text.getLocalBounds();
    startButton.text.setOrigin(stRect.left + stRect.width / 2.f,
        stRect.top + stRect.height / 2.f);
    startButton.text.setPosition(startButton.shape.getPosition() +
        startButton.shape.getSize() / 2.f);

    // Кнопка "Exit"
    exitButton.shape.setSize({ 200, 60 });
    exitButton.shape.setPosition(winSize.x / 2.f - 100, winSize.y / 2.f + 60);
    exitButton.text.setFont(PixelFont);
    exitButton.text.setString("Exit");
    exitButton.text.setCharacterSize(30);
    exitButton.text.setFillColor(sf::Color::White);
    exitButton.setFillColor(sf::Color::Black);
    sf::FloatRect exRect = exitButton.text.getLocalBounds();
    exitButton.text.setOrigin(exRect.left + exRect.width / 2.f,
        exRect.top + exRect.height / 2.f);
    exitButton.text.setPosition(exitButton.shape.getPosition() +
        exitButton.shape.getSize() / 2.f);

    // ==================================================================
    // ГЛАВНЫЙ ИГРОВОЙ ЦИКЛ
    // ==================================================================
    while (window.isOpen()) {
        // Вычисляем время, прошедшее с предыдущего кадра (delta time)
        float deltaTime = deltaClock.restart().asSeconds();

        // ---------------------------------------------------------------
        // ОБРАБОТКА СОБЫТИЙ
        // ---------------------------------------------------------------
        sf::Event event;
        while (window.pollEvent(event)) {
            // Закрытие окна (крестик или Alt+F4)
            if (event.type == sf::Event::Closed) {
                window.close();
                return 0;
            }

            // Обработка нажатий клавиш
            if (event.type == sf::Event::KeyPressed) {
                // Взаимодействие с NPC (Enter), только если персонаж касается Фриска
                if (event.key.code == sf::Keyboard::Enter) {
                    if (NPCcheckCollision(SansSprite, FriskSprite)) {
                        dialogOpened = true;
                        FriskTalk.play();
                    }
                }
            }

            // Обработка навигации и выбора в главном меню (только клавиатура)
            if (currentState == GameState::MENU) {
                if (event.type == sf::Event::KeyPressed) {
                    // Движение вверх по меню (W или стрелка вверх)
                    if (event.key.code == sf::Keyboard::W ||
                        event.key.code == sf::Keyboard::Up) {
                        firstMegalovania.play(); // Звук перемещения
                        selectedButton = (selectedButton - 1 + 2) % 2; // Циклический переход 0 <-> 1
                    }
                    // Движение вниз (S или стрелка вниз)
                    if (event.key.code == sf::Keyboard::S ||
                        event.key.code == sf::Keyboard::Down) {
                        firstMegalovania.play();
                        selectedButton = (selectedButton + 1) % 2;
                    }
                    // Активация выбранной кнопки (Enter или Space)
                    if (event.key.code == sf::Keyboard::Enter ||
                        event.key.code == sf::Keyboard::Space) {
                        selectSound.play(); // Звук подтверждения
                        if (selectedButton == 0) {
                            // --- Запуск игры ---
                            currentState = GameState::PLAYING;
                            menuMusic.stop(); // Останавливаем музыку меню

                            // Запускаем музыку, соответствующую текущей локации
                            if (currentLocation == GameLocation::FIRST_LOCATION) {
                                sansTheme.play();
                                sansTheme.setLoop(true);
                            }
                            else if (currentLocation == GameLocation::SNOW_FOREST) {
                                snowMusic.play();
                                snowMusic.setLoop(true);
                            }

                            previousLocation = currentLocation; // Запоминаем локацию для отслеживания смен

                            // Центрируем камеру на игроке
                            gameView.setCenter(SansSprite.getPosition());
                        }
                        else {
                            // --- Выход из игры ---
                            window.close();
                        }
                    }
                }
            }
        } // конец обработки событий

        // Флаг: движется ли игрок в этом кадре (нужен для анимации)
        bool isMoving = false;

        // ---------------------------------------------------------------
        // ГЛОБАЛЬНЫЕ УПРАВЛЯЮЩИЕ КЛАВИШИ (независимо от состояния)
        // ---------------------------------------------------------------
        // ESC — возврат в главное меню из игры
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Escape)) {
            if (currentState == GameState::PLAYING) {
                currentState = GameState::MENU;
                sansTheme.stop();   // Заглушаем всю игровую музыку
                snowMusic.stop();
                menuMusic.play();   // Включаем музыку меню
                menuMusic.setLoop(true);
            }
        }

        // ---------------------------------------------------------------
        // ИГРОВАЯ ЛОГИКА (выполняется только в состоянии PLAYING)
        // ---------------------------------------------------------------
        if (currentState == GameState::PLAYING) {
            sf::Vector2f oldPos = SansSprite.getPosition(); // Запоминаем позицию до движения (для отката при коллизии)

            sf::Vector2f movement(0.0f, 0.0f);

            // Считываем нажатия клавиш и формируем вектор движения
            // W - вверх, S - вниз, A - влево, D - вправо
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::W)) {
                movement.y -= baseSpeed * speedMultiplier;
                currentRow = 2; // Вверх (строка 2 в спрайт-листе)
                isMoving = true;
            }
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::S)) {
                movement.y += baseSpeed * speedMultiplier;
                currentRow = 0; // Вниз
                isMoving = true;
            }
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::A)) {
                movement.x -= baseSpeed * speedMultiplier;
                currentRow = 3; // Влево
                isMoving = true;
            }
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::D)) {
                movement.x += baseSpeed * speedMultiplier;
                currentRow = 1; // Вправо
                isMoving = true;
            }

            // Применяем движение с учётом deltaTime (плавность)
            SansSprite.move(movement * deltaTime);

            // Обновление анимации
            if (isMoving) {
                // Если прошло достаточно времени, переключаем кадр
                if (animClock.getElapsedTime().asSeconds() >= animationSpeed) {
                    currentFrame = (currentFrame + 1) % 8; // Всего 8 кадров анимации
                    animClock.restart();
                }
            }
            else {
                currentFrame = 0; // Когда стоим на месте — показываем нулевой кадр (idle)
            }

            // Устанавливаем вырезанный прямоугольник текстуры (нужный кадр)
            SansSprite.setTextureRect(sf::IntRect(
                currentFrame * frameWidth,
                currentRow * frameHeight,
                frameWidth,
                frameHeight
            ));

            if (dialogOpened && !NPCcheckCollision(SansSprite, FriskSprite)) {
                dialogOpened = false;
                FriskTalk.stop();
            }

            // ---------- ЛОГИКА ПЕРВОЙ ЛОКАЦИИ ----------
            if (currentLocation == GameLocation::FIRST_LOCATION) {
                speedMultiplier = 1.0f; // Нормальная скорость

                // Если игрок касается препятствия-перехода – переносим в лес
                if (checkCollision(SansSprite, obstacles)) {
                    currentLocation = GameLocation::SNOW_FOREST;
                    firstForestLoad = true;
                    // Безопасная позиция в лесу (по центру почти вверху карты)
                    SansSprite.setPosition(forestBounds.width / 2.f, forestBounds.height / 2.f - 750);
                }

                // Ограничиваем перемещение игрока границами первой карты
                sf::Vector2f playerPos = SansSprite.getPosition();
                float originX = SansSprite.getOrigin().x * SansSprite.getScale().x;
                float originY = SansSprite.getOrigin().y * SansSprite.getScale().y;

                if (playerPos.x < mapBounds.left + originX)
                    playerPos.x = mapBounds.left + originX;
                if (playerPos.x > mapBounds.left + mapBounds.width - originX)
                    playerPos.x = mapBounds.left + mapBounds.width - originX;
                if (playerPos.y < mapBounds.top + originY)
                    playerPos.y = mapBounds.top + originY;
                if (playerPos.y > mapBounds.top + mapBounds.height - originY)
                    playerPos.y = mapBounds.top + mapBounds.height - originY;
                SansSprite.setPosition(playerPos);

                // Камера следует за игроком, но не выходит за границы карты
                float halfViewW = gameView.getSize().x / 2.f;
                float halfViewH = gameView.getSize().y / 2.f;
                float centerX = std::clamp(SansSprite.getPosition().x,
                    mapBounds.left + halfViewW,
                    mapBounds.left + mapBounds.width - halfViewW);
                float centerY = std::clamp(SansSprite.getPosition().y,
                    mapBounds.top + halfViewH,
                    mapBounds.top + mapBounds.height - halfViewH);
                gameView.setCenter(centerX, centerY);
            }
            // ---------- ЛОГИКА СНЕЖНОГО ЛЕСА ----------
            else if (currentLocation == GameLocation::SNOW_FOREST) {
                speedMultiplier = 0.7f; // Замедляем игрока на 30% (снег)

                dialogOpened = false; // В лесу диалог не показывается (Фриска нет)

                // Проверка столкновения с препятствиями леса (вертикальные стены)
                if (checkCollision(SansSprite, forestObstacles)) {
                    SansSprite.setPosition(oldPos); // Возвращаем на предыдущую позицию
                }

                // Если игрок заходит в зону возврата – переносим в первую локацию
                if (checkCollision(SansSprite, forestReturnObstacles)) {
                    currentLocation = GameLocation::FIRST_LOCATION;
                    firstForestLoad = false;
                    SansSprite.setPosition(950, 350); // Позиция после возврата
                }

                // Ограничение по краям лесной карты (аналогично первой)
                sf::Vector2f playerPos = SansSprite.getPosition();
                float originX = SansSprite.getOrigin().x * SansSprite.getScale().x;
                float originY = SansSprite.getOrigin().y * SansSprite.getScale().y;

                if (playerPos.x < forestBounds.left + originX)
                    playerPos.x = forestBounds.left + originX;
                if (playerPos.x > forestBounds.left + forestBounds.width - originX)
                    playerPos.x = forestBounds.left + forestBounds.width - originX;
                if (playerPos.y < forestBounds.top + originY)
                    playerPos.y = forestBounds.top + originY;
                if (playerPos.y > forestBounds.top + forestBounds.height - originY)
                    playerPos.y = forestBounds.top + forestBounds.height - originY;
                SansSprite.setPosition(playerPos);

                // Камера для леса
                float halfViewW = gameView.getSize().x / 2.f;
                float halfViewH = gameView.getSize().y / 2.f;
                float centerX = std::clamp(SansSprite.getPosition().x,
                    forestBounds.left + halfViewW,
                    forestBounds.left + forestBounds.width - halfViewW);
                float centerY = std::clamp(SansSprite.getPosition().y,
                    forestBounds.top + halfViewH,
                    forestBounds.top + forestBounds.height - halfViewH);
                gameView.setCenter(centerX, centerY);
            }

            // ---------- СМЕНА МУЗЫКИ ПРИ ПЕРЕХОДЕ МЕЖДУ ЛОКАЦИЯМИ ----------
            if (currentLocation != previousLocation) {
                // Если локация сменилась – переключаем музыкальный трек
                if (currentLocation == GameLocation::FIRST_LOCATION) {
                    snowMusic.stop();
                    sansTheme.play();
                    sansTheme.setLoop(true);
                }
                else if (currentLocation == GameLocation::SNOW_FOREST) {
                    sansTheme.stop();
                    snowMusic.play();
                    snowMusic.setLoop(true);
                }
                previousLocation = currentLocation; // Обновляем сохранённую локацию
            }
        }

        // ---------- Визуальное выделение кнопок меню ----------
        if (currentState == GameState::MENU) {
            if (selectedButton == 0) {
                startButton.text.setFillColor(sf::Color::Yellow); // Активная кнопка — жёлтый текст
                exitButton.text.setFillColor(sf::Color::White);
            }
            else {
                startButton.text.setFillColor(sf::Color::White);
                exitButton.text.setFillColor(sf::Color::Yellow);
            }
        }

        // ===============================================================
        // ОТРИСОВКА КАДРА
        // ===============================================================
        window.clear(sf::Color::Black); // Очищаем экран чёрным

        if (currentState == GameState::MENU) {
            // Для меню используем стандартный вид (без игровой камеры)
            window.setView(window.getDefaultView());
            window.draw(startButton.shape);
            window.draw(startButton.text);
            window.draw(exitButton.shape);
            window.draw(exitButton.text);
        }
        else if (currentState == GameState::PLAYING) {
            // Устанавливаем игровую камеру
            window.setView(gameView);

            // Рисуем объекты в зависимости от текущей локации
            if (currentLocation == GameLocation::FIRST_LOCATION) {
                window.draw(First_LocationSprite); // Карта
                window.draw(gameTitle);            // Название игры
                window.draw(FriskSprite);          // NPC Фриск

                if (dialogOpened) {                // Если диалог активирован – показываем текст
                    window.draw(NPCTitle);
                }

                // Отображаем все препятствия (обычно они прозрачные)
                for (const auto& obstacle : obstacles) {
                    window.draw(obstacle);
                }
            }
            else if (currentLocation == GameLocation::SNOW_FOREST) {
                window.draw(snow_ForestSprite);    // Карта леса

                // Препятствия леса (вертикальные стены)
                for (const auto& obstacle : forestObstacles)
                    window.draw(obstacle);
                // Препятствие для возврата
                for (const auto& obstacle : forestReturnObstacles)
                    window.draw(obstacle);
            }

            // Игрок рисуется поверх всего
            window.draw(SansSprite);
        }

        window.display(); // Показываем сформированный кадр
    }
    return 0;
}