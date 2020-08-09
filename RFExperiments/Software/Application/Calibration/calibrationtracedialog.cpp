#include "calibrationtracedialog.h"
#include "ui_calibrationtracedialog.h"
#include "measurementmodel.h"
#include <QStyle>

CalibrationTraceDialog::CalibrationTraceDialog(Calibration *cal, Calibration::Type type) :
    QDialog(nullptr),
    ui(new Ui::CalibrationTraceDialog),
    cal(cal),
    requestedType(type)
{
    ui->setupUi(this);
    ui->bApply->setIcon(style()->standardIcon(QStyle::SP_DialogApplyButton));
    measurements = cal->Measurements(type);
    if(requestedType == Calibration::Type::None) {
        ui->bApply->setVisible(false);
    }
    model = new MeasurementModel(cal, measurements);
    ui->tableView->setModel(model);
    ui->tableView->setColumnWidth(0, 100);
    ui->tableView->setColumnWidth(1, 350);
    ui->tableView->setColumnWidth(2, 320);
    ui->tableView->setColumnWidth(3, 160);
    UpdateApplyButton();
}

CalibrationTraceDialog::~CalibrationTraceDialog()
{
    delete ui;
}

void CalibrationTraceDialog::measurementComplete(Calibration::Measurement m)
{
    model->measurementUpdated(m);
    UpdateApplyButton();
}

void CalibrationTraceDialog::UpdateApplyButton()
{
    ui->bApply->setEnabled(cal->calculationPossible(requestedType));
}

void CalibrationTraceDialog::on_bDelete_clicked()
{
    auto measurement = measurements[ui->tableView->currentIndex().row()];
    cal->clearMeasurement(measurement);
    model->measurementUpdated(measurement);
    UpdateApplyButton();
}

void CalibrationTraceDialog::on_bMeasure_clicked()
{
    auto measurement = measurements[ui->tableView->currentIndex().row()];
    emit triggerMeasurement(measurement);
}

void CalibrationTraceDialog::on_bApply_clicked()
{
    emit applyCalibration(requestedType);
    accept();
}
