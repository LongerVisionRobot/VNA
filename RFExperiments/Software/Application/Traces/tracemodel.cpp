#include "tracemodel.h"
#include <QIcon>

using namespace std;

TraceModel::TraceModel(QObject *parent)
    : QAbstractTableModel(parent)
{
    traces.clear();
}

void TraceModel::addTrace(Trace *t)
{
    beginInsertRows(QModelIndex(), traces.size(), traces.size());
    traces.push_back(t);
    endInsertRows();
    emit traceAdded(t);
}

void TraceModel::removeTrace(int index)
{
    if (index < traces.size()) {
        beginRemoveRows(QModelIndex(), index, index);
        auto trace = traces[index];
        delete trace;
        traces.erase(traces.begin() + index);
        endRemoveRows();
        emit traceRemoved(trace);
    }
}

Trace *TraceModel::trace(int index)
{
    return traces.at(index);
}

void TraceModel::toggleVisibility(int index)
{
    if (index < traces.size()) {
        traces[index]->setVisible(!traces[index]->isVisible());
        emit dataChanged(createIndex(index, 0), createIndex(index, 0));
    }
}

void TraceModel::togglePause(int index)
{
    if (index < traces.size()) {
        if(traces[index]->isPaused()) {
            traces[index]->resume();
        } else {
            traces[index]->pause();
        }
        emit dataChanged(createIndex(index, 1), createIndex(index, 1));
    }
}

int TraceModel::rowCount(const QModelIndex &parent) const
{
    return traces.size();
}

int TraceModel::columnCount(const QModelIndex &parent) const
{
    return 3;
}

QVariant TraceModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if (index.row() >= traces.size())
        return QVariant();
    if (index.column() == 0) {
        if (role == Qt::DecorationRole) {
            if (traces[index.row()]->isVisible()) {
                return QIcon(":/icons/visible.svg");
            } else {
                return QIcon(":/icons/invisible.svg");
            }
        } else {
            return QVariant();
        }
    } else if (index.column() == 1) {
        if (role == Qt::DecorationRole && !traces[index.row()]->isTouchstone()) {
            if (traces[index.row()]->isPaused()) {
                return QIcon(":/icons/pause.svg");
            } else {
                return QIcon(":/icons/play.svg");
            }
        } else {
            return QVariant();
        }
    } else if (index.column() == 2) {
        if (role == Qt::DisplayRole) {
            return traces[index.row()]->name();
        } else if (role == Qt::ForegroundRole) {
            return traces[index.row()]->color();
        } else {
            return QVariant();
        }
    } else {
        return QVariant();
    }
}

std::vector<Trace *> TraceModel::getTraces()
{
    return traces;
}

void TraceModel::clearVNAData()
{
    for(auto t : traces) {
        if (!t->isTouchstone()) {
            // this trace is fed from live data
            t->clear();
        }
    }
}

void TraceModel::addVNAData(Protocol::Datapoint d)
{
    for(auto t : traces) {
        if (!t->isTouchstone()) {
            Trace::Data td;
            td.frequency = d.frequency;
            switch(t->liveParameter()) {
            case Trace::LiveParameter::S11: td.S = complex<double>(d.real_S11, d.imag_S11); break;
            case Trace::LiveParameter::S12: td.S = complex<double>(d.real_S12, d.imag_S12); break;
            case Trace::LiveParameter::S21: td.S = complex<double>(d.real_S21, d.imag_S21); break;
            case Trace::LiveParameter::S22: td.S = complex<double>(d.real_S22, d.imag_S22); break;
            }
            t->addData(td);
        }
    }
}
