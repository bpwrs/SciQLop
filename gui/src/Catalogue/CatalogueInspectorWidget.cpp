#include "Catalogue/CatalogueInspectorWidget.h"
#include "ui_CatalogueInspectorWidget.h"

#include <Common/DateUtils.h>
#include <DBCatalogue.h>
#include <DBEvent.h>
#include <DBEventProduct.h>
#include <DBTag.h>

struct CatalogueInspectorWidget::CatalogueInspectorWidgetPrivate {
    std::shared_ptr<DBCatalogue> m_DisplayedCatalogue = nullptr;
    std::shared_ptr<DBEvent> m_DisplayedEvent = nullptr;
    std::shared_ptr<DBEventProduct> m_DisplayedEventProduct = nullptr;

    void connectCatalogueUpdateSignals(CatalogueInspectorWidget *inspector,
                                       Ui::CatalogueInspectorWidget *ui);
    void connectEventUpdateSignals(CatalogueInspectorWidget *inspector,
                                   Ui::CatalogueInspectorWidget *ui);
};

CatalogueInspectorWidget::CatalogueInspectorWidget(QWidget *parent)
        : QWidget(parent),
          ui(new Ui::CatalogueInspectorWidget),
          impl{spimpl::make_unique_impl<CatalogueInspectorWidgetPrivate>()}
{
    ui->setupUi(this);
    showPage(Page::Empty);

    impl->connectCatalogueUpdateSignals(this, ui);
    impl->connectEventUpdateSignals(this, ui);

    ui->dateTimeEventTStart->setDisplayFormat(DATETIME_FORMAT);
    ui->dateTimeEventTEnd->setDisplayFormat(DATETIME_FORMAT);
}

CatalogueInspectorWidget::~CatalogueInspectorWidget()
{
    delete ui;
}

void CatalogueInspectorWidget::CatalogueInspectorWidgetPrivate::connectCatalogueUpdateSignals(
    CatalogueInspectorWidget *inspector, Ui::CatalogueInspectorWidget *ui)
{
    connect(ui->leCatalogueName, &QLineEdit::editingFinished, [ui, inspector, this]() {
        if (ui->leCatalogueName->text() != m_DisplayedCatalogue->getName()) {
            m_DisplayedCatalogue->setName(ui->leCatalogueName->text());
            emit inspector->catalogueUpdated(m_DisplayedCatalogue);
        }
    });

    connect(ui->leCatalogueAuthor, &QLineEdit::editingFinished, [ui, inspector, this]() {
        if (ui->leCatalogueAuthor->text() != m_DisplayedCatalogue->getAuthor()) {
            m_DisplayedCatalogue->setAuthor(ui->leCatalogueAuthor->text());
            emit inspector->catalogueUpdated(m_DisplayedCatalogue);
        }
    });
}

void CatalogueInspectorWidget::CatalogueInspectorWidgetPrivate::connectEventUpdateSignals(
    CatalogueInspectorWidget *inspector, Ui::CatalogueInspectorWidget *ui)
{
    connect(ui->leEventName, &QLineEdit::editingFinished, [ui, inspector, this]() {
        if (ui->leEventName->text() != m_DisplayedEvent->getName()) {
            m_DisplayedEvent->setName(ui->leEventName->text());
            emit inspector->eventUpdated(m_DisplayedEvent);
        }
    });

    connect(ui->leEventTags, &QLineEdit::editingFinished, [ui, inspector, this]() {
        auto tags = ui->leEventTags->text().split(QRegExp("\\s+"), QString::SkipEmptyParts);
        std::list<QString> tagNames;
        for (auto tag : tags) {
            tagNames.push_back(tag);
        }

        if (m_DisplayedEvent->getTagsNames() != tagNames) {
            m_DisplayedEvent->setTagsNames(tagNames);
            emit inspector->eventUpdated(m_DisplayedEvent);
        }
    });

    connect(ui->leEventProduct, &QLineEdit::editingFinished, [ui, inspector, this]() {
        if (ui->leEventProduct->text() != m_DisplayedEventProduct->getProductId()) {
            auto oldProductId = m_DisplayedEventProduct->getProductId();
            m_DisplayedEventProduct->setProductId(ui->leEventProduct->text());

            auto eventProducts = m_DisplayedEvent->getEventProducts();
            for (auto &eventProduct : eventProducts) {
                if (eventProduct.getProductId() == oldProductId) {
                    eventProduct.setProductId(m_DisplayedEventProduct->getProductId());
                }
            }
            m_DisplayedEvent->setEventProducts(eventProducts);

            emit inspector->eventUpdated(m_DisplayedEvent);
        }
    });

    connect(ui->dateTimeEventTStart, &QDateTimeEdit::editingFinished, [ui, inspector, this]() {
        auto time = DateUtils::secondsSinceEpoch(ui->dateTimeEventTStart->dateTime());
        if (time != m_DisplayedEventProduct->getTStart()) {
            m_DisplayedEventProduct->setTStart(time);

            auto eventProducts = m_DisplayedEvent->getEventProducts();
            for (auto &eventProduct : eventProducts) {
                if (eventProduct.getProductId() == m_DisplayedEventProduct->getProductId()) {
                    eventProduct.setTStart(m_DisplayedEventProduct->getTStart());
                }
            }
            m_DisplayedEvent->setEventProducts(eventProducts);

            emit inspector->eventUpdated(m_DisplayedEvent);
        }
    });

    connect(ui->dateTimeEventTEnd, &QDateTimeEdit::editingFinished, [ui, inspector, this]() {
        auto time = DateUtils::secondsSinceEpoch(ui->dateTimeEventTEnd->dateTime());
        if (time != m_DisplayedEventProduct->getTEnd()) {
            m_DisplayedEventProduct->setTEnd(time);

            auto eventProducts = m_DisplayedEvent->getEventProducts();
            for (auto &eventProduct : eventProducts) {
                if (eventProduct.getProductId() == m_DisplayedEventProduct->getProductId()) {
                    eventProduct.setTEnd(m_DisplayedEventProduct->getTEnd());
                }
            }
            m_DisplayedEvent->setEventProducts(eventProducts);

            emit inspector->eventUpdated(m_DisplayedEvent);
        }
    });
}

void CatalogueInspectorWidget::showPage(CatalogueInspectorWidget::Page page)
{
    ui->stackedWidget->setCurrentIndex(static_cast<int>(page));
}

CatalogueInspectorWidget::Page CatalogueInspectorWidget::currentPage() const
{
    return static_cast<Page>(ui->stackedWidget->currentIndex());
}

void CatalogueInspectorWidget::setEvent(const std::shared_ptr<DBEvent> &event)
{
    impl->m_DisplayedEvent = event;

    blockSignals(true);

    showPage(Page::EventProperties);
    ui->leEventName->setEnabled(true);
    ui->leEventName->setText(event->getName());
    ui->leEventProduct->setEnabled(false);

    auto eventProducts = event->getEventProducts();
    QStringList eventProductList;
    for (auto evtProduct : eventProducts) {
        eventProductList << evtProduct.getProductId();
    }

    ui->leEventProduct->setText(eventProductList.join(";"));

    QString tagList;
    auto tags = event->getTagsNames();
    for (auto tag : tags) {
        tagList += tag;
        tagList += ' ';
    }

    ui->leEventTags->setEnabled(true);
    ui->leEventTags->setText(tagList);

    ui->dateTimeEventTStart->setEnabled(false);
    ui->dateTimeEventTEnd->setEnabled(false);

    ui->dateTimeEventTStart->setDateTime(DateUtils::dateTime(event->getTStart()));
    ui->dateTimeEventTEnd->setDateTime(DateUtils::dateTime(event->getTEnd()));

    blockSignals(false);
}

void CatalogueInspectorWidget::setEventProduct(const std::shared_ptr<DBEvent> &event,
                                               const std::shared_ptr<DBEventProduct> &eventProduct)
{

    impl->m_DisplayedEvent = event;
    impl->m_DisplayedEventProduct = eventProduct;

    blockSignals(true);

    showPage(Page::EventProperties);
    ui->leEventName->setEnabled(false);
    ui->leEventName->setText(event->getName());
    ui->leEventProduct->setEnabled(false);
    ui->leEventProduct->setText(eventProduct->getProductId());

    ui->leEventTags->setEnabled(false);
    ui->leEventTags->clear();

    ui->dateTimeEventTStart->setEnabled(true);
    ui->dateTimeEventTEnd->setEnabled(true);

    ui->dateTimeEventTStart->setDateTime(DateUtils::dateTime(eventProduct->getTStart()));
    ui->dateTimeEventTEnd->setDateTime(DateUtils::dateTime(eventProduct->getTEnd()));

    blockSignals(false);
}

void CatalogueInspectorWidget::setCatalogue(const std::shared_ptr<DBCatalogue> &catalogue)
{
    impl->m_DisplayedCatalogue = catalogue;

    blockSignals(true);

    showPage(Page::CatalogueProperties);
    ui->leCatalogueName->setText(catalogue->getName());
    ui->leCatalogueAuthor->setText(catalogue->getAuthor());

    blockSignals(false);
}

void CatalogueInspectorWidget::refresh()
{
    switch (static_cast<Page>(ui->stackedWidget->currentIndex())) {
        case Page::CatalogueProperties:
            setCatalogue(impl->m_DisplayedCatalogue);
            break;
        case Page::EventProperties: {
            auto isEventShowed = ui->leEventName->isEnabled();
            setEvent(impl->m_DisplayedEvent);
            if (!isEventShowed && impl->m_DisplayedEvent) {
                setEventProduct(impl->m_DisplayedEvent, impl->m_DisplayedEventProduct);
            }
        }
    }
}