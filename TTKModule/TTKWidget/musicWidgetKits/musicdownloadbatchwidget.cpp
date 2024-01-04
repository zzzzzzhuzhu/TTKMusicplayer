#include "musicdownloadbatchwidget.h"
#include "ui_musicdownloadbatchwidget.h"
#include "musicdownloadrecordconfigmanager.h"
#include "musicdownloadmetadatarequest.h"
#include "musicnumberutils.h"

MusicDownloadBatchTableItem:: MusicDownloadBatchTableItem(QWidget *parent)
    : QWidget(parent),
      m_networkRequest(nullptr)
{
    m_songName = new QLabel(this);
    m_singer = new QLabel(this);
    m_information = new QLabel(this);
    m_information->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_status = new QLabel(this);
    m_status->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    m_qulity = new QComboBox(this);
    TTK::Widget::generateComboBoxFormat(m_qulity, TTK::UI::ComboBoxStyle02 + TTK::UI::ItemView01);

    m_songName->setGeometry(0, 0, 190, TTK_ITEM_SIZE_S);
    m_singer->setGeometry(180, 0, 120, TTK_ITEM_SIZE_S);
    m_qulity->setGeometry(300, 0, 80, TTK_ITEM_SIZE_S);
    m_information->setGeometry(380, 0, 150, TTK_ITEM_SIZE_S);
    m_status->setGeometry(530, 0, TTK_ITEM_SIZE_S, TTK_ITEM_SIZE_S);
    m_queryType = MusicAbstractQueryRequest::QueryType::Music;

    connect(m_qulity, SIGNAL(currentIndexChanged(int)), SLOT(currentQualityChanged(int)));
}

MusicDownloadBatchTableItem::~MusicDownloadBatchTableItem()
{
    delete m_songName;
    delete m_singer;
    delete m_information;
    delete m_status;
    delete m_qulity;
}

void MusicDownloadBatchTableItem::addCellItem(MusicAbstractQueryRequest *request, const TTK::MusicSongInformation &info)
{
    m_songName->setToolTip(info.m_songName);
    m_songName->setText(TTK::Widget::elidedText(m_songName->font(), m_songName->toolTip(), Qt::ElideRight, m_songName->width() - 10));

    m_singer->setToolTip(info.m_singerName);
    m_singer->setText(TTK::Widget::elidedText(m_singer->font(), m_singer->toolTip(), Qt::ElideRight, m_singer->width() - 10));

    m_songInfo = info;
    m_networkRequest = request;
    m_queryType = request->queryType();

    m_qulity->addItems({QObject::tr("SD"), QObject::tr("HQ"), QObject::tr("SQ"), QObject::tr("CD")});
    m_qulity->setCurrentIndex(0);
}

void MusicDownloadBatchTableItem::startToRequest()
{
    if(!G_NETWORK_PTR->isOnline() || m_qulity->currentIndex() < 0)
    {
        return;
    }

    if(m_queryType == MusicAbstractQueryRequest::QueryType::Music)
    {
        startRequestMusic();
    }
    else if(m_queryType == MusicAbstractQueryRequest::QueryType::Movie)
    {
        startRequestMovie();
    }
}

void MusicDownloadBatchTableItem::setCurrentQuality(int index)
{
    m_qulity->setCurrentIndex(index);
}

void MusicDownloadBatchTableItem::currentQualityChanged(int index)
{
    const int bitrate = currentBitrate(index);
    if(m_queryType == MusicAbstractQueryRequest::QueryType::Music)
    {
        const QString &size = TTK::Number::sizeByteToLabel(m_songInfo.m_duration, bitrate);
        m_information->setText(QString("%1/%2KBPS/%3").arg(size).arg(bitrate).arg(QString(bitrate > TTK_BN_320 ? FLAC_FILE_SUFFIX : MP3_FILE_SUFFIX).toUpper()));
    }
    else if(m_queryType == MusicAbstractQueryRequest::QueryType::Movie)
    {
        const QString &size = TTK::Number::sizeByteToLabel(m_songInfo.m_duration, bitrate);
        m_information->setText(QString("%1/%2KBPS/%3").arg(size).arg(bitrate).arg(QString(MP4_FILE_SUFFIX).toUpper()));
    }

    m_status->setPixmap(QPixmap(":/tiny/lb_question"));
}

void MusicDownloadBatchTableItem::dataDownloadFinished()
{
    m_status->setPixmap(QPixmap(":/tiny/lb_right"));
}

int MusicDownloadBatchTableItem::currentBitrate(int index)
{
    int bitrate = -1;
    if(m_queryType == MusicAbstractQueryRequest::QueryType::Music)
    {
        switch(index)
        {
            case 0: bitrate = TTK_BN_128; break;
            case 1: bitrate = TTK_BN_192; break;
            case 2: bitrate = TTK_BN_320; break;
            case 3: bitrate = TTK_BN_1000; break;
            default: break;
        }
    }
    else if(m_queryType == MusicAbstractQueryRequest::QueryType::Movie)
    {
        switch(index)
        {
            case 0: bitrate = TTK_BN_250; break;
            case 1: bitrate = TTK_BN_500; break;
            case 2: bitrate = TTK_BN_750; break;
            case 3: bitrate = TTK_BN_1000; break;
            default: break;
        }
    }
    return bitrate;
}

void MusicDownloadBatchTableItem::startRequestMusic()
{
    const int bitrate = currentBitrate(m_qulity->currentIndex());
    m_networkRequest->startToQueryResult(&m_songInfo, bitrate);
    if(m_songInfo.m_songProps.isEmpty())
    {
        return;
    }

    TTK::MusicSongProperty prop;
    for(const TTK::MusicSongProperty &p : qAsConst(m_songInfo.m_songProps))
    {
        if(p.m_bitrate == bitrate)
        {
            prop = p;
            break;
        }
    }

    if(prop.isEmpty())
    {
        return;
    }

    QString musicSong = m_singer->toolTip() + " - " + m_songName->toolTip();
    const QString &downloadPrefix = G_SETTING_PTR->value(MusicSettingManager::DownloadMusicDirPath).toString();
    QString downloadName = QString("%1%2.%3").arg(downloadPrefix, musicSong, prop.m_format);

    MusicDownloadRecordConfigManager manager;
    if(!manager.fromFile(TTK::toString(TTK::Record::NormalDownload)))
    {
        return;
    }

    MusicSongList records;
    manager.readBuffer(records);

    MusicSong record;
    record.setName(musicSong);
    record.setPath(QFileInfo(downloadName).absoluteFilePath());
    record.setSizeStr(prop.m_size);
    record.setAddTimeStr("-1");
    records << record;

    manager.reset();
    manager.writeBuffer(records);

    if(QFile::exists(downloadName))
    {
        for(int i = 1; i < 99; ++i)
        {
            if(!QFile::exists(downloadName))
            {
                break;
            }
            if(i != 1)
            {
                musicSong.chop(3);
            }
            musicSong += QString("(%1)").arg(i);
            downloadName = QString("%1%2.%3").arg(downloadPrefix, musicSong, prop.m_format);
        }
    }

    MusicDownloadMetaDataRequest *d = new MusicDownloadMetaDataRequest(prop.m_url, downloadName, this);
    d->setRecordType(TTK::Record::NormalDownload);
    connect(d, SIGNAL(downLoadDataChanged(QString)), this, SLOT(dataDownloadFinished()));

    MusicSongMeta meta;
    meta.setComment(m_songInfo.m_coverUrl);
    meta.setTitle(m_songInfo.m_songName);
    meta.setArtist(m_songInfo.m_singerName);
    meta.setAlbum(m_songInfo.m_albumName);
    meta.setTrackNum(m_songInfo.m_trackNumber);
    meta.setYear(m_songInfo.m_year);

    d->setSongMeta(meta);
    d->startToRequest();
}

void MusicDownloadBatchTableItem::startRequestMovie()
{
    const int bitrate = currentBitrate(m_qulity->currentIndex());
    m_networkRequest->startToQueryResult(&m_songInfo, bitrate);
    if(m_songInfo.m_songProps.isEmpty())
    {
        return;
    }

    TTK::MusicSongProperty prop;
    for(const TTK::MusicSongProperty &p : qAsConst(m_songInfo.m_songProps))
    {
        if(p.m_bitrate == bitrate)
        {
            prop = p;
            break;
        }
    }

    if(prop.isEmpty())
    {
        return;
    }

    const QString &downloadPrefix = MOVIE_DIR_FULL;
    QString musicSong = m_singer->toolTip() + " - " + m_songName->toolTip();
    //
    QString downloadName = QString("%1%2.%3").arg(downloadPrefix, musicSong, prop.m_format);
    if(QFile::exists(downloadName))
    {
        for(int i = 1; i < 99; ++i)
        {
            if(!QFile::exists(downloadName))
            {
                break;
            }
            if(i != 1)
            {
                musicSong.chop(3);
            }
            musicSong += QString("(%1)").arg(i);
            downloadName = QString("%1%2.%3").arg(downloadPrefix, musicSong, prop.m_format);
        }
    }
    //
    MusicDownloadDataRequest *d = new MusicDownloadDataRequest(prop.m_url, downloadName, TTK::Download::Video, this);
    connect(d, SIGNAL(downLoadDataChanged(QString)), this, SLOT(dataDownloadFinished()));
    d->startToRequest();
}



MusicDownloadBatchTableWidget::MusicDownloadBatchTableWidget(QWidget *parent)
    : MusicAbstractTableWidget(parent)
{
    setColumnCount(1);

    QHeaderView *headerview = horizontalHeader();
    headerview->resizeSection(0, 555);

    setStyleSheet(styleSheet() + TTK::UI::TableWidgetStyle02);
    verticalScrollBar()->setStyleSheet(TTK::UI::ScrollBarStyle01);

    TTK::Widget::setTransparent(this, 255);
#if defined Q_OS_UNIX && !TTK_QT_VERSION_CHECK(5,7,0) //Fix linux selection-background-color stylesheet bug
    TTK::Widget::setTransparent(this, QColor(220, 220, 220));
#endif
}

MusicDownloadBatchTableWidget::~MusicDownloadBatchTableWidget()
{
    removeItems();
}

void MusicDownloadBatchTableWidget::addCellItem(MusicAbstractQueryRequest *request, const TTK::MusicSongInformation &info)
{
    const int index = rowCount();
    setRowCount(index + 1);
    setRowHeight(index, TTK_ITEM_SIZE_S);

    QTableWidgetItem *it = new QTableWidgetItem;
    setItem(index, 0,  it);

    MusicDownloadBatchTableItem *item = new MusicDownloadBatchTableItem(this);
    item->addCellItem(request, info);
    setCellWidget(index, 0, item);
    m_items << item;
}

void MusicDownloadBatchTableWidget::removeItems()
{
    qDeleteAll(m_items);
    m_items.clear();

    MusicAbstractTableWidget::removeItems();
    setColumnCount(1);
}

void MusicDownloadBatchTableWidget::currentQualityChanged(int index)
{
    for(MusicDownloadBatchTableItem *item : qAsConst(m_items))
    {
        ///Remove first null item object
        item->setCurrentQuality(index);
    }
}

void MusicDownloadBatchTableWidget::startToRequest()
{
    for(MusicDownloadBatchTableItem *item : qAsConst(m_items))
    {
        item->startToRequest();
    }
}



MusicDownloadBatchWidget::MusicDownloadBatchWidget(QWidget *parent)
    : MusicAbstractMoveDialog(parent),
      m_ui(new Ui::MusicDownloadBatchWidget)
{
    m_ui->setupUi(this);
    setFixedSize(size());
    setBackgroundLabel(m_ui->background);

    m_ui->topTitleCloseButton->setIcon(QIcon(":/functions/btn_close_hover"));
    m_ui->topTitleCloseButton->setStyleSheet(TTK::UI::ToolButtonStyle04);
    m_ui->topTitleCloseButton->setCursor(QCursor(Qt::PointingHandCursor));
    m_ui->topTitleCloseButton->setToolTip(tr("Close"));
    connect(m_ui->topTitleCloseButton, SIGNAL(clicked()), SLOT(close()));

    TTK::Widget::generateComboBoxFormat(m_ui->qualityBox, TTK::UI::ComboBoxStyle02 + TTK::UI::ItemView01);
    m_ui->qualityBox->addItems({QObject::tr("SD"), QObject::tr("HQ"), QObject::tr("SQ"), QObject::tr("CD")});
    connect(m_ui->qualityBox, SIGNAL(currentIndexChanged(int)), m_ui->tableWidget, SLOT(currentQualityChanged(int)));

    m_ui->qualityBox->setCurrentIndex(0);

    m_ui->downloadButton->setStyleSheet(TTK::UI::PushButtonStyle05);
#ifdef Q_OS_UNIX
    m_ui->downloadButton->setFocusPolicy(Qt::NoFocus);
#endif

    connect(m_ui->downloadButton, SIGNAL(clicked()), m_ui->tableWidget, SLOT(startToRequest()));
}

MusicDownloadBatchWidget::~MusicDownloadBatchWidget()
{
    delete m_ui;
}

void MusicDownloadBatchWidget::setSongName(MusicAbstractQueryRequest *request, const TTKIntList &items)
{
    TTK::MusicSongInformationList infos, songInfos(request->songInfoList());
    for(int index : qAsConst(items))
    {
        if(index < 0 || index >= songInfos.count())
        {
            continue;
        }

        infos << songInfos[index];
    }

    for(const TTK::MusicSongInformation &info : qAsConst(infos))
    {
        m_ui->tableWidget->addCellItem(request, info);
    }
    m_ui->songCountLabel->setText(tr("All Songs Count %1").arg(infos.count()));
}
