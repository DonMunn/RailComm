#include "railcomm.h"

RailComm::RailComm(QObject *parent) : SerialComm(parent) {
    connect(&serial_conn, &QSerialPort::readyRead, this, &RailComm::serialConnReceiveMessage);
    connect(&move_status_timer, &QTimer::timeout, this, &RailComm::sendPosStatusCommand);
    move_status_timer.setSingleShot(true);
}

void RailComm::setEcho(bool status) {
    if (isOpen()) {
        if(!command_queue.isEmpty()) {
            command_queue.enqueue(commands::SETECHO);
            data_queue.enqueue(QString::number((int)status));
        } else {
            command_queue.enqueue(commands::SETECHO);
            data_queue.enqueue(QString::number((int)status));
            if (wait_for_move == false)
                serialConnSendMessage();
        }
    } else {
        sendError(QSerialPort::NotOpenError, "No open connection");
    }
}

void RailComm::setVerbosity(bool status) {
    if (isOpen()) {
        if(!command_queue.isEmpty()) {
            command_queue.enqueue(commands::SETVERBOSITY);
            data_queue.enqueue(QString::number((int)status));
        } else {
            command_queue.enqueue(commands::SETVERBOSITY);
            data_queue.enqueue(QString::number((int)status));
            if (wait_for_move == false)
                serialConnSendMessage();
        }
    } else {
        sendError(QSerialPort::NotOpenError, "No open connection");
    }
}

void RailComm::setUserUnit(const QString &unit) {
    if (isOpen()) {
        if(!command_queue.isEmpty()) {
            command_queue.enqueue(commands::SETUSERUNIT);
            data_queue.enqueue(unit);
        } else {
            command_queue.enqueue(commands::SETUSERUNIT);
            data_queue.enqueue(unit);
            if (wait_for_move == false)
                serialConnSendMessage();
        }
    } else {
        sendError(QSerialPort::NotOpenError, "No open connection");
    }
}

void RailComm::setStartVelocity(int mm) {
    if (isOpen()) {
        if(!command_queue.isEmpty()) {
            command_queue.enqueue(commands::SETSTARTVELOCITY);
            data_queue.enqueue(QString::number(mm));
        } else {
            command_queue.enqueue(commands::SETSTARTVELOCITY);
            data_queue.enqueue(QString::number(mm));
            if (wait_for_move == false)
                serialConnSendMessage();
        }
    } else {
        sendError(QSerialPort::NotOpenError, "No open connection");
    }
}

void RailComm::setEndVelocity(int mm) {
    if (isOpen()) {
        if(!command_queue.isEmpty()) {
            command_queue.enqueue(commands::SETENDVELOCITY);
            data_queue.enqueue(QString::number(mm));
        } else {
            command_queue.enqueue(commands::SETENDVELOCITY);
            data_queue.enqueue(QString::number(mm));
            if (wait_for_move == false)
                serialConnSendMessage();
        }
    } else {
        sendError(QSerialPort::NotOpenError, "No open connection");
    }
}

void RailComm::setAccelTime(double seconds) {
    if (isOpen()) {
        if(!command_queue.isEmpty()) {
            command_queue.enqueue(commands::SETACCELTIME);
            data_queue.enqueue(QString::number(seconds));
        } else {
            command_queue.enqueue(commands::SETACCELTIME);
            data_queue.enqueue(QString::number(seconds));
            if (wait_for_move == false)
                serialConnSendMessage();
        }
    } else {
        sendError(QSerialPort::NotOpenError, "No open connection");
    }
}

void RailComm::setDeccelTime(double seconds) {
    if (isOpen()) {
        if(!command_queue.isEmpty()) {
            command_queue.enqueue(commands::SETDECCELTIME);
            data_queue.enqueue(QString::number(seconds));
        } else {
            command_queue.enqueue(commands::SETDECCELTIME);
            data_queue.enqueue(QString::number(seconds));
            if (wait_for_move == false)
                serialConnSendMessage();
        }
    } else {
        sendError(QSerialPort::NotOpenError, "No open connection");
    }
}

void RailComm::moveHome() {
    if (isOpen()) {
        if(!command_queue.isEmpty()) {
            command_queue.enqueue(commands::MOVEHOME);
        } else {
            command_queue.enqueue(commands::MOVEHOME);
            if (wait_for_move == false)
                serialConnSendMessage();
        }
    } else {
        sendError(QSerialPort::NotOpenError, "No open connection");
    }
}

void RailComm::moveAbsolute(int mm) {
    if (isOpen()) {
        if(!command_queue.isEmpty()) {
            command_queue.enqueue(commands::MOVEABSOLUTE);
            data_queue.enqueue(QString::number(mm));
        } else {
            command_queue.enqueue(commands::MOVEABSOLUTE);
            data_queue.enqueue(QString::number(mm));
            if (wait_for_move == false)
                serialConnSendMessage();
        }
    } else {
        sendError(QSerialPort::NotOpenError, "No open connection");
    }
}


//Private
void RailComm::serialConnSendMessage() {
    commands command = (commands)command_queue.head();

    if (command == commands::MOVEHOME || command == commands::MOVEABSOLUTE) {
        wait_for_move = true;
    }

    QByteArray data = getCommand(command).toUtf8();

    if (containsData(command)) {
        if (command == commands::SETECHO || command == commands::SETVERBOSITY) {
            data += '=';
        } else {
            data += ' ';
        }

        data += data_queue.head().toUtf8();
    }

    data += '\n';

    qDebug() << "final data:" << data;

    if (serial_conn.write(data) == -1) { // -1 indicates error occurred
        // send QSerialPort::NotOpenError if QOIDevice::NotOpen is triggered
        if (serial_conn.error() == QSerialPort::NoError) {
            sendError(QSerialPort::NotOpenError, "No open connection");
        } //else UNNEEDED as the QSerialPort will emit its own signal for other errors
    } else {
        emit rawDataSignal(data);
        timer.start(1000);
    }
}

bool RailComm::containsData(commands command) {
    switch (command) {
        case commands::SETECHO:
        case commands::SETVERBOSITY:
        case commands::SETUSERUNIT:
        case commands::SETSTARTVELOCITY:
        case commands::SETENDVELOCITY:
        case commands::SETACCELTIME:
        case commands::SETDECCELTIME:
        case commands::HOMESTATUS:
        case commands::MOVEABSOLUTE:
        case commands::CONTROLLERREADY:
            return true;
    }

    return false;
}

QString RailComm::getCommand(commands command) {
    switch (command) {
        case commands::NONE:
            return "";
        case commands::SETECHO:
            return "ECHO";
        case commands::SETVERBOSITY:
            return "VERBOSE";
        case commands::SETUSERUNIT:
            return "UU";
        case commands::SETSTARTVELOCITY:
            return "VS";
        case commands::SETENDVELOCITY:
            return "VR";
        case commands::SETACCELTIME:
            return "TA";
        case commands::SETDECCELTIME:
            return "TD";
        case commands::MOVEHOME:
            return "MGH";
        case commands::HOMESTATUS:
            return "SIGHOMEP";
        case commands::MOVEABSOLUTE:
            return "MA";
        case commands::CONTROLLERREADY:
            return "SIGREADY";
    }
}

void RailComm::sendError(QSerialPort::SerialPortError error, const QString &error_message) {
    // clear serial internal read/write buffers
    if (isOpen()) {
        serial_conn.clear();
    }

    // Dequeue command if one is associated with the error
    if (command_queue.isEmpty()) {
        emit errorSignal(error, error_message, commands::NONE);
    } else {
        commands command = (commands)command_queue.dequeue();
        if (containsData(command))
            data_queue.dequeue();

        temp_data = "";

        if (command == commands::MOVEHOME || command == commands::MOVEABSOLUTE || command == commands::CONTROLLERREADY || command == commands::HOMESTATUS) {
            wait_for_move = false;
            move_status_timer.stop();
        }
        emit errorSignal(error, error_message, command);
    }
}

//Slots
void RailComm::serialConnReceiveMessage() {
    // construct message from parts
    temp_data += serial_conn.readAll();

    // regex matching all commands and SIGHOMEP or SIGREADY successful return
    if(QRegExp(getCommand((commands)command_queue.head())).exactMatch(temp_data)) {
        timer.stop();
        commands command = (commands)command_queue.dequeue();

        if (command == commands::CONTROLLERREADY || command == commands::HOMESTATUS) {
            move_status_timer.stop();
            wait_for_move = false;
        }

        if (containsData(command))
            data_queue.dequeue();

        emit returnData(temp_data, command);

        temp_data = "";
        // Start sending CONTROLLERREADY or HOMESTATUS commands
        if (command == commands::MOVEHOME || command == commands::MOVEABSOLUTE) {
            status_command = command;
            move_status_timer.start(0);
        }
        // send another message when previous is finished if the move status does not need to be obtained
        else if (!command_queue.isEmpty()) {
            serialConnSendMessage();
        }
    } else if (false) { //regex matching a status of 0 being returned from SIGHOMEP or SIGREADY
        timer.stop();
        commands command = (commands)command_queue.dequeue();

        if (containsData(command))
            data_queue.dequeue();

        emit returnData(temp_data, command);

        temp_data = "";

        move_status_timer.start(250);
    }
}

void RailComm::sendPosStatusCommand() {
    if (wait_for_move == true) {
        if (status_command == commands::MOVEHOME) {
            command_queue.prepend(commands::HOMESTATUS);
        } else if (status_command == commands::MOVEABSOLUTE) {
            command_queue.prepend(commands::CONTROLLERREADY);
        }

        serialConnSendMessage();
    }
}

