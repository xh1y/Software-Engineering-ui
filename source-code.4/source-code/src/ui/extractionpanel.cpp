#include "extractionpanel.h"

#include <QTextEdit>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFont>
#include <QFontMetrics>
#include <QScrollBar>
#include <QStyle>

namespace optikg {

ExtractionPanel::ExtractionPanel(QWidget *parent)
    : QWidget(parent)
    , textEdit_(nullptr)
    , extractBtn_(nullptr)
    , clearBtn_(nullptr)
    , charCountLabel_(nullptr) {
    setupUi();
}

void ExtractionPanel::setupUi() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    // 增加四周的留白 (Margin) 和控件间距 (Spacing)
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(15);

    // 标题
    QLabel* titleLabel = new QLabel(tr("输入维修文本 (最多 %1 字符)").arg(MAX_CHARS));
    titleLabel->setObjectName("titleLabel");
    mainLayout->addWidget(titleLabel);

    // 文本编辑框
    textEdit_ = new QTextEdit();
    textEdit_->setPlaceholderText(tr("请输入发动机异响、液压系统故障等维修描述文本..."));
    textEdit_->setMaximumHeight(150);
    // 去掉硬编码的字体，交给 QSS 接管
    textEdit_->setAcceptRichText(false);
    connect(textEdit_, &QTextEdit::textChanged, this, &ExtractionPanel::onTextChanged);
    mainLayout->addWidget(textEdit_);

    // 字符计数
    charCountLabel_ = new QLabel(tr("字符数: 0/%1").arg(MAX_CHARS));
    charCountLabel_->setObjectName("charCountLabel");
    charCountLabel_->setAlignment(Qt::AlignRight);
    mainLayout->addWidget(charCountLabel_);

    // 按钮布局
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->setSpacing(10);

    extractBtn_ = new QPushButton(tr("开始抽取"));
    extractBtn_->setObjectName("extractBtn");
    // 样式由 QSS 文件控制
    extractBtn_->setCursor(Qt::PointingHandCursor);
    connect(extractBtn_, &QPushButton::clicked, this, &ExtractionPanel::extractClicked);

    clearBtn_ = new QPushButton(tr("清空"));
    clearBtn_->setObjectName("clearBtn");
    // 样式由 QSS 文件控制
    clearBtn_->setCursor(Qt::PointingHandCursor);
    connect(clearBtn_, &QPushButton::clicked, this, &ExtractionPanel::clearClicked);

    buttonLayout->addStretch();
    buttonLayout->addWidget(clearBtn_);
    buttonLayout->addWidget(extractBtn_);

    mainLayout->addLayout(buttonLayout);
    mainLayout->addStretch();

    // 初始更新字符计数
    updateCharCount();
}

QString ExtractionPanel::text() const {
    return textEdit_->toPlainText().trimmed();
}

void ExtractionPanel::setText(const QString& text) {
    textEdit_->setPlainText(text);
    updateCharCount();
}

void ExtractionPanel::clear() {
    textEdit_->clear();
    updateCharCount();
}

void ExtractionPanel::setEnabled(bool enabled) {
    textEdit_->setEnabled(enabled);
    extractBtn_->setEnabled(enabled);
    clearBtn_->setEnabled(enabled);
}

void ExtractionPanel::onTextChanged() {
    updateCharCount();
    emit textChanged(text());
}

void ExtractionPanel::updateCharCount() {
    QString content = textEdit_->toPlainText();
    int length = content.length();

    // 限制最大字符数
    if (length > MAX_CHARS) {
        content = content.left(MAX_CHARS);
        textEdit_->setPlainText(content);
        QTextCursor cursor = textEdit_->textCursor();
        cursor.movePosition(QTextCursor::End);
        textEdit_->setTextCursor(cursor);
        length = MAX_CHARS;
    }

    // 更新标签
    charCountLabel_->setText(tr("字符数: %1/%2").arg(length).arg(MAX_CHARS));

    // 颜色提示 - 使用属性标记状态，由 QSS 控制样式
    if (length >= MAX_CHARS) {
        charCountLabel_->setProperty("state", "error");
        charCountLabel_->setStyleSheet("color: #f44336; font-weight: bold;");
    } else if (length > MAX_CHARS * 0.9) {
        charCountLabel_->setProperty("state", "warning");
        charCountLabel_->setStyleSheet("color: #FF9800; font-weight: bold;");
    } else {
        charCountLabel_->setProperty("state", "normal");
        charCountLabel_->setStyleSheet("color: #666666;");
    }
    charCountLabel_->style()->unpolish(charCountLabel_);
    charCountLabel_->style()->polish(charCountLabel_);
}

} // namespace optikg