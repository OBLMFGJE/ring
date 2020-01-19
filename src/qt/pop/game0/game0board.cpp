// Copyright (c) 2019 The Ring Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

// Ring-fork: Pop

#include <algorithm>
#include <ctime>

#include <QMessageBox>
#include <QPainter>
#include <QPixmap>
#include <QLayout>
#include <QResizeEvent>
#include <QCheckBox>
#include <QPushButton>
#include <QFile>
#include <QVBoxLayout>

#include <qt/pop/soundplayer.h>
#include <qt/pop/availablegamestablemodel.h>
#include <qt/pop/game0/game0board.h>
#include <qt/uicolours.h>

#include <chainparams.h>

Game0Board::Game0Board(QWidget* parent)
:	QWidget(parent),
	done(true),
    soundsEnabled(true),
    autoSubmit(true)
{
	setMinimumSize(200, 200);
	setFocusPolicy(Qt::StrongFocus);
	setFocus();
    setMouseTracking(true);

    // Create thumbs
    for (int i = 0; i < GAME0_NUM_TILES; i++)
        tileImageThumbs[i] = QPixmap(":/game0/boardtile" + QString::number(i)).scaled(TILE_THUMB_SIZE, TILE_THUMB_SIZE, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

	// Create next tile preview
	preview = new QLabel();
	preview->setFixedSize(100, 100);
	preview->setAutoFillBackground(true);
    preview->setAlignment(Qt::AlignCenter);
	{
		QPalette palette = preview->palette();
		palette.setColor(preview->backgroundRole(), QColor(SKIN_BG_PANEL));
		preview->setPalette(palette);
	}

	// Create score display
	scoreLabel = new QLabel("0");
	scoreLabel->setAlignment(Qt::AlignCenter);
    scoreLabel->setStyleSheet("font-size: 36px; color: " + SKIN_TEXT + ";");
    scoreMessage = new QLabel();
    scoreMessage->setStyleSheet("color: " + SKIN_TEXT + ";");
    scoreMessage->setFixedWidth(100);
    scoreMessage->setWordWrap(true);

	// Create sound toggle
	soundToggle = new QCheckBox("Enable sounds");
    soundToggle->setChecked(true);
    soundToggle->setStyleSheet("color: " + SKIN_TEXT + ";");
    connect(soundToggle, &QCheckBox::clicked, this, &Game0Board::on_soundToggle_clicked);

	// Create auto submit toggle
	autoSubmitToggle = new QCheckBox("Autosubmit on win");
    autoSubmitToggle->setChecked(true);
    autoSubmitToggle->setStyleSheet("color: " + SKIN_TEXT + ";");
    connect(autoSubmitToggle, &QCheckBox::clicked, this, &Game0Board::on_autoSubmitToggle_clicked);

	// Create restart button
	restartButton = new QPushButton("Restart");
    restartButton->setEnabled(false);
    restartButton->setIcon(QIcon(":/icons/refresh"));
    connect(restartButton, &QPushButton::clicked, this, &Game0Board::on_restartButton_clicked);

	// Create submit button
	submitButton = new QPushButton("Submit");
    submitButton->setEnabled(false);
    submitButton->setIcon(QIcon(":/icons/send"));
    connect(submitButton, &QPushButton::clicked, this, &Game0Board::on_submitButton_clicked);

	// Create overlay message
	QLabel* message = new QLabel(tr("To begin, choose an available game from the left-hand list."), this);
	message->setAttribute(Qt::WA_TransparentForMouseEvents);
	message->setAlignment(Qt::AlignCenter);
	message->setStyleSheet(
		"QLabel {"
			"background-color: rgba(30, 30, 30, 200);"
			"color: white;"
			"margin: 0;"
			"padding: 0.5em;"
			"border-radius: 0.5em;"
		"}");
	message->setWordWrap(true);
	connect(this, &Game0Board::showMessage, message, &QLabel::show);
	connect(this, &Game0Board::showMessage, message, &QLabel::setText);
	connect(this, &Game0Board::hideMessage, message, &QLabel::hide);
	connect(this, &Game0Board::hideMessage, message, &QLabel::clear);

    // Misc labels
    QLabel *gameLabel = new QLabel("Game");
    gameLabel->setAlignment(Qt::AlignCenter);
    gameLabel->setStyleSheet("color: " + SKIN_TEXT + ";");
    gameHashLabel = new QLabel("N / A");
    gameHashLabel->setAlignment(Qt::AlignCenter);
    gameHashLabel->setStyleSheet("color: " + SKIN_TEXT + ";");

    QLabel *timeLeftCaptionLabel = new QLabel("Time left");
    timeLeftCaptionLabel->setAlignment(Qt::AlignCenter);
    timeLeftCaptionLabel->setStyleSheet("color: " + SKIN_TEXT + ";");
    timeLeftLabel = new QLabel("N / A");
    timeLeftLabel->setAlignment(Qt::AlignCenter);
    timeLeftLabel->setStyleSheet("color: " + SKIN_TEXT + ";");    

    QLabel *nextTileLabel = new QLabel("Next Tile");
    nextTileLabel->setAlignment(Qt::AlignCenter);
    nextTileLabel->setStyleSheet("color: " + SKIN_TEXT + ";");
    
    QLabel *scoreTitleLabel = new QLabel("Score");
    scoreTitleLabel->setAlignment(Qt::AlignCenter);
    scoreTitleLabel->setStyleSheet("color: " + SKIN_TEXT + ";");

    QLabel *scoreTargetLabel1 = new QLabel("Target");
    scoreTargetLabel1->setAlignment(Qt::AlignCenter);
    scoreTargetLabel1->setStyleSheet("color: " + SKIN_TEXT + ";");

    QLabel *scoreTargetLabel2 = new QLabel(QString::number(GAME0_SCORE_TARGET));
    scoreTargetLabel2->setAlignment(Qt::AlignCenter);
    scoreTargetLabel2->setStyleSheet("color: " + SKIN_TEXT + ";");    

	// Assemble layout
	QGridLayout* layout = new QGridLayout;
	layout->setContentsMargins(4, 4, 4, 4);
    layout->setSpacing(8);
	
    layout->addWidget(this, 0, 0, -1, 1);
	layout->addWidget(message, 0, 0, -1, 1);

    int row = 0;

    layout->addWidget(nextTileLabel, row++, 1);
    layout->addWidget(preview, row++, 1);

	layout->addWidget(scoreTitleLabel, row++, 1);
    layout->addWidget(scoreLabel, row++, 1);
    layout->addWidget(scoreTargetLabel1, row++, 1);
    layout->addWidget(scoreTargetLabel2, row++, 1);

    layout->addWidget(scoreMessage, row++, 1);
    
    layout->addWidget(new QLabel(""), row++, 1);    // Blank expander
    layout->setRowStretch(row - 1, 1);

    layout->addWidget(soundToggle, row++, 1);
    layout->addWidget(autoSubmitToggle, row++, 1);
    
    layout->addWidget(new QLabel(""), row++, 1);    // Blank expander
    layout->setRowStretch(row - 1, 1);

    layout->addWidget(gameLabel, row++, 1);
    layout->addWidget(gameHashLabel, row++, 1);
    layout->addWidget(timeLeftCaptionLabel, row++, 1);
    layout->addWidget(timeLeftLabel, row++, 1);

    layout->addWidget(new QLabel(""), row++, 1);    // Blank expander
    layout->setRowStretch(row - 1, 1);

    layout->addWidget(restartButton, row++, 1);
    layout->addWidget(submitButton, row++, 1);

    parent->setLayout(layout);
}

bool Game0Board::endGame() {
	if (done || QMessageBox::question(this, "Are you sure?", "Really end the current game?", QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::Yes)
		return true;

    return false;
}

bool Game0Board::newGame(const CAvailableGame *newGame) {
    // Confirm abandoning game if in progress
    if (!endGame())
		return false;

    // Reset everything
    Q_EMIT hideMessage();
	done = false;
	score = 0;
	scoreLabel->setText(QString::number(score));
    scoreMessage->setText("");    
    currentTileRotation = 0;
    restartButton->setEnabled(true);
    submitButton->setEnabled(false);

    // Init the game and get first two tiles
    currentGame = newGame;
    game.InitGame(currentGame->gameSourceHash);
    gameHashLabel->setText(QString::fromStdString(currentGame->gameSourceHash.ToString().substr(0,8) + "..."));
    preview->setPixmap(tileImageThumbs[game.GetNextTileType()]);

    return true;
}

void Game0Board::mouseMoveEvent(QMouseEvent* event) {
    // Update highlighted tile
    if (this->rect().contains(event->pos())) {
        QPoint mousePos = event->pos();
        highlightedTileX = (mousePos.x() - background.x()) / tileSize;
        highlightedTileY = (mousePos.y() - background.y()) / tileSize;
    } else
        highlightedTileX = highlightedTileY = -1;

    update();   // Redraw
}

void Game0Board::mousePressEvent(QMouseEvent* event) {
	if (done)
        return;

    if (event->button() == Qt::RightButton) {                   // Rotate on right-click
        rotateTile();
        if (soundsEnabled) JustPlay(":/game0/rotate");
    } else                                                    // Try to place tile on left-click
        placeTile();
}

void Game0Board::rotateTile() {    
    currentTileRotation = (currentTileRotation + 1) % 4;        // Rotate current
    update();                                                   // Redraw
}

bool Game0Board::placeTile() {
    // Try and place the tile
    std::string strError;
    if (!game.PlaceTile(highlightedTileX, highlightedTileY, currentTileRotation, strError)) {
        if (soundsEnabled) JustPlay(":/game0/badclick");
        return false;
    }

    currentTileRotation = 0;
    preview->setPixmap(tileImageThumbs[game.GetNextTileType()]);

    // Redraw
    update();

    // Update score
    int oldScore = score;
    std::string strDesc;
    score = game.CalculateScore(strDesc);
    scoreLabel->setText(QString::number(score));
    scoreMessage->setText(QString(strDesc.c_str()));

    // Check for win condition
    if (score >= GAME0_SCORE_TARGET) {
        if (soundsEnabled) JustPlay(":/game0/win");
    	done = true;
    	if (autoSubmit) Q_EMIT solutionReady(currentGame, 0, game.GetSolution());
        Q_EMIT showMessage(tr("<big><b>Game Won!</b></big>"));
       	restartButton->setEnabled(false);
        submitButton->setEnabled(true);
        return true;
    }

    // Check for lose condition (out of room)
    if (game.GetTilesPlaced() == GAME0_BOARD_SIZE * GAME0_BOARD_SIZE) {
        if (soundsEnabled) JustPlay(":/game0/lose");
        done = true;
	    Q_EMIT showMessage(tr("<big><b>Game Over; target not met.</b></big>"));
        return true;
    }

    if (soundsEnabled)
        JustPlay(score > oldScore ? ":/game0/score" : ":/game0/goodclick");

    return true;
}

void Game0Board::paintEvent(QPaintEvent*) {
	QPainter painter(this);
	painter.setRenderHint(QPainter::Antialiasing, true);
	painter.fillRect(background, QColor(SKIN_BG_TAB));

	// Draw board
    std::vector<std::pair<int,int>> candles = game.GetCandles();
	for (int y = 0; y < GAME0_BOARD_SIZE; y++)
		for (int x = 0; x < GAME0_BOARD_SIZE; x++) {
            Game0Tile t = game.GetTileAt(x, y);
            int screenX = x * tileSize + background.x(), screenY = y * tileSize + background.y();
            if (!done && x == highlightedTileX && y == highlightedTileY && t.tileTypeIndex == -1) {
                painter.drawPixmap(screenX, screenY, tileImages[game.GetCurrentTileType() + currentTileRotation * GAME0_NUM_TILES]);
                painter.drawPixmap(screenX, screenY, (game.GetTilesPlaced() == 0 || game.HasNeighbour(x, y)) ? mouseOverImageOK : mouseOverImageBad);
            } else if (t.tileTypeIndex > -1)
				painter.drawPixmap(screenX, screenY, tileImages[t.tileTypeIndex + t.rotation * GAME0_NUM_TILES]);
            if (std::find(candles.begin(), candles.end(), std::make_pair(x, y)) != candles.end())
                painter.drawPixmap(screenX, screenY, candleImage);
		}
}

void Game0Board::resizeEvent(QResizeEvent* event) {
    // Regenerate images
	tileSize = std::min(event->size().width() / GAME0_BOARD_SIZE, event->size().height() / GAME0_BOARD_SIZE);
	int w = tileSize * GAME0_BOARD_SIZE + 1, h = tileSize * GAME0_BOARD_SIZE + 1;
	background = QRect((width() - w) / 2, (height() - h) / 2, w, h);

    mouseOverImageOK = QPixmap(":/game0/boardmouseoverok").scaled(tileSize, tileSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    mouseOverImageBad = QPixmap(":/game0/boardmouseoverbad").scaled(tileSize, tileSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    candleImage = QPixmap(":/game0/candle").scaled(tileSize, tileSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

    QMatrix mat90, mat180, mat270;
    mat90.rotate(90);
    mat180.rotate(180);
    mat270.rotate(270);

    for (int i = 0; i < GAME0_NUM_TILES; i++) {
        QPixmap orig(":/game0/boardtile" + QString::number(i));
        orig = orig.scaled(tileSize, tileSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        tileImages[i] = orig;
        tileImages[i + GAME0_NUM_TILES] = orig.transformed(mat90);
        tileImages[i + GAME0_NUM_TILES * 2] = orig.transformed(mat180);
        tileImages[i + GAME0_NUM_TILES * 3] = orig.transformed(mat270);
    }
}

void Game0Board::updateTimeLeft(int blocks) {
    if (!timeLeftLabel)
        return;

    if (blocks < 0)
        timeLeftLabel->setText("EXPIRED!");
    else
        timeLeftLabel->setText(QString::number(blocks) + " blocks (" + AvailableGamesTableModel::secondsToString(blocks * Params().GetConsensus().nPowTargetSpacing / 2) + ")");
}

void Game0Board::on_restartButton_clicked() {
    newGame(currentGame);
}

void Game0Board::on_submitButton_clicked() {
    Q_EMIT solutionReady(currentGame, 0, game.GetSolution());
}

void Game0Board::on_soundToggle_clicked() {
    soundsEnabled = !soundsEnabled;
}

void Game0Board::on_autoSubmitToggle_clicked() {
    autoSubmit = !autoSubmit;
}
