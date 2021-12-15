#ifndef PUMPCOMM_H
#define PUMPCOMM_H

#include "serialcomm.h"
#include "settingsdialog.h"


class RailComm : public SerialComm
{
    Q_OBJECT
public:
    enum commands {NONE, SETECHO, SETVERBOSITY, SETUSERUNIT, SETSTARTVELOCITY, SETENDVELOCITY, SETACCELTIME,
                   SETDECCELTIME, MOVEHOME, MOVEABSOLUTE, CONTROLLERREADY};

    explicit RailComm(QObject *parent = nullptr);

    void setEcho(bool status);
    void setVerbosity(bool status);
    void setUserUnit(const QString &unit);
    void setStartVelocity(int mm);
    void setEndVelocity(int mm);
    void setAccelTime(double seconds);
    void setDeccelTime(double seconds);
    void moveHome();
    void moveAbsolute(int mm);

private:
    void sendError(QSerialPort::SerialPortError error, const QString &error_message) override; // Done
    void serialConnSendMessage() override; // Done
    QString getCommand(commands command); // Done
    bool containsData(commands command); // Done
    QByteArray constructMessage(bool regex=false); // Done

    QString user_unit;
    bool wait_for_move = false;
    QTimer move_status_timer;

private slots:
    void serialConnReceiveMessage() override; //Done TODO match regexes
    void sendMessage() override;
    void sendPosStatusCommand(); // Done
};

#endif // PUMPCOMM_H
