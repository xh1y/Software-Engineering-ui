#pragma once

#include <QWidget>

QT_BEGIN_NAMESPACE
class QTextEdit;
class QPushButton;
class QLabel;
QT_END_NAMESPACE

namespace optikg {

class ExtractionPanel : public QWidget {
    Q_OBJECT

public:
    explicit ExtractionPanel(QWidget *parent = nullptr);

    QString text() const;
    void setText(const QString& text);
    void clear();
    void setEnabled(bool enabled);

signals:
    void extractClicked();
    void clearClicked();
    void textChanged(const QString& text);

private slots:
    void onTextChanged();
    void updateCharCount();

private:
    void setupUi();

    QTextEdit* textEdit_;
    QPushButton* extractBtn_;
    QPushButton* clearBtn_;
    QLabel* charCountLabel_;

    static constexpr int MAX_CHARS = 5000;
};

} // namespace optikg