#include "unit.h"
#include <math.h>
#include <sstream>
#include <iomanip>
#include <QDebug>

using namespace std;

double Unit::FromString(QString string, QString unit, QString prefixes)
{
    if(string.size() == 0) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    // remove unit if present
    if(string.endsWith(unit, Qt::CaseInsensitive)) {
        string.chop(unit.size());
    }
    // check if last char is a valid prefix
    double factor = 1.0;
    if(prefixes.contains(string.at(string.size()-1))) {
        QChar prefix = string.at(string.size()-1);
        factor = SIPrefixToFactor(prefix.toLatin1());
        string.chop(1);
    }
    bool convertOk;
    auto value = string.toDouble(&convertOk);
    if(!convertOk) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    return value * factor;
}

QString Unit::ToString(double value, QString unit, QString prefixes, int precision)
{
    // change label text
    QString sValue;
    if(isnan(value) || isinf(value)) {
        sValue.append("NaN");
        return sValue;
    } else if(value == 0.0) {
        sValue.append("0 ");
    } else {
        if(value < 0) {
            sValue.append('-');
            value = -value;
        }
        int preDotDigits = log10(value) + 2;
        int prefixIndex = prefixes.indexOf(' ');
        while(preDotDigits > 3 && prefixIndex < prefixes.length() - 1) {
            value /= 1000.0;
            preDotDigits -= 3;
            prefixIndex++;
        }
        while(preDotDigits<=1 && prefixIndex > 0) {
            value *= 1000.0;
            preDotDigits += 3;
            prefixIndex--;
        }
        stringstream ss;
        ss << std::fixed;
        if(preDotDigits >= 0) {
            if(precision - preDotDigits + 1 < 0) {
                ss << std::setprecision(0);
            } else {
                ss << std::setprecision(precision - preDotDigits + 1);
            }
        } else {
            ss << std::setprecision(precision - 1);
        }
        ss << value;
        sValue.append(QString::fromStdString(ss.str()));
        sValue.append(prefixes[prefixIndex]);
    }
    sValue.append(unit);
    return sValue;
}

double Unit::SIPrefixToFactor(char prefix)
{
    switch(prefix) {
    case 'f': return 1e-15; break;
    case 'p': return 1e-12; break;
    case 'n': return 1e-9; break;
    case 'u': return 1e-6; break;
    case 'm': return 1e-3; break;
    case ' ': return 1e0; break;
    case 'k': return 1e3; break;
    case 'M': return 1e6; break;
    case 'G': return 1e9; break;
    case 'T': return 1e12; break;
    case 'P': return 1e15; break;
    default: return 0; break;
    }
}
