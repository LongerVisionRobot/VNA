#include "siunitedit.h"

#include <QDoubleValidator>
#include <unit.h>
#include <QEvent>
#include <QKeyEvent>

SIUnitEdit::SIUnitEdit(QString unit, QString prefixes, int precision, QWidget *parent)
    : QLineEdit(parent)
{
    this->unit = unit;
    this->prefixes = prefixes;
    this->precision = precision;
    setAlignment(Qt::AlignCenter);
    installEventFilter(this);
    setValidator(new QDoubleValidator);
    connect(this, &QLineEdit::editingFinished, [this]() {
       parseNewValue(1.0);
    });
}

SIUnitEdit::SIUnitEdit(QWidget *parent)
    : SIUnitEdit("", " ", 4, parent)
{

}

void SIUnitEdit::setValue(double value)
{
    setValueQuiet(value);
    emit valueChanged(value);
}

bool SIUnitEdit::eventFilter(QObject *, QEvent *event)
{
    if (event->type() == QEvent::KeyPress) {
        int key = static_cast<QKeyEvent *>(event)->key();
        if(key == Qt::Key_Escape) {
            // abort editing process and set old value
            setValueQuiet(_value);
            return true;
        }
        if(key == Qt::Key_Return) {
            // use new value without prefix
           parseNewValue(1.0);
           return true;
        }
        auto mod = static_cast<QKeyEvent *>(event)->modifiers();
        if (!(mod & Qt::ShiftModifier)) {
            key = tolower(key);
        }
        if(key <= 255 && prefixes.indexOf(key) >= 0) {
            // a valid prefix key was pressed
            parseNewValue(Unit::SIPrefixToFactor(key));
            return true;
        }
    } else if(event->type() == QEvent::FocusOut) {
        setValueQuiet(_value);
    }
    return false;
}

void SIUnitEdit::setValueQuiet(double value)
{
    _value = value;
    clear();
    setPlaceholderText(Unit::ToString(value, unit, prefixes, precision));
    clearFocus();
}

void SIUnitEdit::parseNewValue(double factor)
{
    double v = text().toDouble() * factor;
    setValue(v);
}
