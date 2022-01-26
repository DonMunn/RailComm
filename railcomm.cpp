#include "railcomm.h"

RailComm::RailComm(QObject *parent) : SerialComm(parent) {
    connect(&serial_conn, &QSerialPort::readyRead, this, &RailComm::serialConnReceiveMessage);
    connect(&move_status_timer, &QTimer::timeout, this, &RailComm::sendPosStatusCommand);
    connect(&send_message_timer, &QTimer::timeout, this, &RailComm::sendMessage);
    move_status_timer.setSingleShot(true);
}

void RailComm::setEcho(bool status) {
    if (isOpen()) {
        command_queue.enqueue(commands::SETECHO);
        data_queue.enqueue(QString::number((int)status));
    } else {
        sendError(QSerialPort::NotOpenError, "No open connection");
    }
}

void RailComm::setVerbosity(bool status) {
    if (isOpen()) {
        command_queue.enqueue(commands::SETVERBOSITY);
        data_queue.enqueue(QString::number((int)status));
    } else {
        sendError(QSerialPort::NotOpenError, "No open connection");
    }
}

void RailComm::setUserUnit(const QString &unit) {
    if (isOpen()) {
        command_queue.enqueue(commands::SETUSERUNIT);
        data_queue.enqueue(unit);
    } else {
        sendError(QSerialPort::NotOpenError, "No open connection");
    }
}

void RailComm::setStartVelocity(int mm) {
    if (isOpen()) {
        command_queue.enqueue(commands::SETSTARTVELOCITY);
        data_queue.enqueue(QString::number(mm));
    } else {
        sendError(QSerialPort::NotOpenError, "No open connection");
    }
}

void RailComm::setEndVelocity(int mm) {
    if (isOpen()) {
        command_queue.enqueue(commands::SETENDVELOCITY);
        data_queue.enqueue(QString::number(mm));
    } else {
        sendError(QSerialPort::NotOpenError, "No open connection");
    }
}

void RailComm::setAccelTime(double seconds) {
    if (isOpen()) {
        command_queue.enqueue(commands::SETACCELTIME);
        data_queue.enqueue(QString::number(seconds));
    } else {
        sendError(QSerialPort::NotOpenError, "No open connection");
    }
}

void RailComm::setDeccelTime(double seconds) {
    if (isOpen()) {
        command_queue.enqueue(commands::SETDECCELTIME);
        data_queue.enqueue(QString::number(seconds));
    } else {
        sendError(QSerialPort::NotOpenError, "No open connection");
    }
}

void RailComm::moveHome() {
    if (isOpen()) {
        command_queue.enqueue(commands::MOVEHOME);
    } else {
        sendError(QSerialPort::NotOpenError, "No open connection");
    }
}

void RailComm::moveAbsolute(int mm) {
    if (isOpen()) {
        command_queue.enqueue(commands::MOVEABSOLUTE);
        data_queue.enqueue(QString::number(mm));
    } else {
        sendError(QSerialPort::NotOpenError, "No open connection");
    }
}


//Private
void RailComm::serialConnSendMessage() {
    QByteArray data = constructMessage();

    qDebug() << "final data:" << data;

    if (serial_conn.write(data) == -1) { // -1 indicates error occurred
        // send QSerialPort::NotOpenError if QOIDevice::NotOpen is triggered
        if (serial_conn.error() == QSerialPort::NoError) {
            sendError(QSerialPort::NotOpenError, "No open connection");
        } //else UNNEEDED as the QSerialPort will emit its own signal for other errors
    } else {
        emit rawDataSignal(data);
        timeout_timer.start(1000);
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
        case commands::MOVEABSOLUTE:
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
            return "RUN HOME";
        case commands::MOVEABSOLUTE:
            return "MA";
        case commands::CONTROLLERREADY:
            return "SIGREADY";
    }
}

void RailComm::sendError(QSerialPort::SerialPortError error, const QString &error_message) {
    send_message_timer.stop();

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

        wait_for_move = false;
        move_status_timer.stop();

        emit errorSignal(error, error_message, command);
    }
}

QByteArray RailComm::constructMessage(bool regex) {
    commands command = (commands)command_queue.head();

    if (command == commands::MOVEHOME || command == commands::MOVEABSOLUTE) {
        wait_for_move = true;
    }

    QByteArray data = getCommand(command).toUtf8();
    qDebug() << command;
    if (containsData(command)) {
        if (command == commands::SETECHO || command == commands::SETVERBOSITY || regex == true) {
            data += '=';
        } else {
            data += ' ';
        }

        if (regex) {
            data += '(';
        }

        data += data_queue.head().toUtf8();

        if (regex) {
            data += ')';
        }
    }

    data += '\n';

    return data;
}

//Slots
void RailComm::serialConnReceiveMessage() {
    // construct message from parts
    temp_data += serial_conn.readAll();

    // regex matching all commands and SIGHOMEP or SIGREADY successful return
    //MA has no return
    //RUN HOME has no return

    QString match_string = constructMessage();
    commands command = (commands)command_queue.head();
    if (containsData(command) && command != MOVEABSOLUTE)
        match_string = match_string + "\\s*" + constructMessage(true);

    switch(command) {
        case SETSTARTVELOCITY:
        case SETENDVELOCITY:
            match_string += " " + user_unit + "/sec";
    }

    match_string += "\\s*>$";

    match_string.prepend("^");
    // Remove all instances of '\n' added by the constructMessage command
    match_string.remove("\n");

    QRegExp match_return = QRegExp(match_string);
    QRegExp match_ready = QRegExp("^SIGREADY\\s*SIGREADY=(1)\\s*>$");
    QRegExp match_not_ready = QRegExp("^SIGREADY\\s*SIGREADY=(0)\\s*>$");

    if(match_return.exactMatch(temp_data) || match_ready.exactMatch(temp_data)) {
        timeout_timer.stop();
        command_queue.dequeue();

        if (command == commands::CONTROLLERREADY) {
            move_status_timer.stop();
            wait_for_move = false;
        }

        if (containsData(command)) {
            if (command == commands::SETUSERUNIT) {
                user_unit = data_queue.dequeue();
            } else {
                data_queue.dequeue();
            }
        }

        if (command == commands::CONTROLLERREADY) {
            emit returnData(match_ready.cap(1), command);
        } else if (match_string.contains('(')){
            emit returnData(match_return.cap(1), command);
        } else {
            emit returnData(temp_data, command);
        }


        temp_data = "";
        // Start sending CONTROLLERREADY or HOMESTATUS commands
        if (command == commands::MOVEHOME || command == commands::MOVEABSOLUTE) {
            move_status_timer.start(0);
        }
    } else if (match_not_ready.exactMatch(temp_data)) { //regex matching a status of 0 being returned from SIGREADY
        timeout_timer.stop();
        command_queue.dequeue();

        if (containsData(command))
            data_queue.dequeue();

        emit returnData(match_not_ready.cap(1), command);

        temp_data = "";

        move_status_timer.start(250);
    }
}

void RailComm::sendPosStatusCommand() {
    if (wait_for_move == true) {
        command_queue.prepend(commands::CONTROLLERREADY);

        serialConnSendMessage();
    }
}

void RailComm::sendMessage() {
    if (isOpen() && !command_queue.isEmpty() && !timeout_timer.isActive() && !wait_for_move) {
        serialConnSendMessage();
    }
}

