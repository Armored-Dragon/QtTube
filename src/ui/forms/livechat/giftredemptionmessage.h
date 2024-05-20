#pragma once
#include <QJsonValue>
#include <QWidget>

class QHBoxLayout;
class QLabel;

class GiftRedemptionMessage : public QWidget
{
public:
    GiftRedemptionMessage(const QJsonValue& renderer, QWidget* parent);
private:
    QLabel* authorLabel;
    QHBoxLayout* layout;
    QLabel* messageLabel;
};
