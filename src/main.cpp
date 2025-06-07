#include <QtWidgets>

class RecoveryWindow : public QMainWindow {
    Q_OBJECT
public:
    RecoveryWindow() {
        stacked = new QStackedWidget(this);
        setCentralWidget(stacked);
        createPartitionPage();
        createActionPage();
        stacked->setCurrentWidget(partitionPage);
        resize(800, 600);
    }
private slots:
    void selectPartition() {
        targetRoot = partitionList->currentItem() ? partitionList->currentItem()->text() : QString();
        if(targetRoot.isEmpty()) return;
        stacked->setCurrentWidget(actionPage);
    }
    void runAction() {
        QString action = actionList->currentItem() ? actionList->currentItem()->text() : QString();
        if (action.isEmpty()) return;

        QString program("arch-chroot");
        QStringList args;
        args << targetRoot;

        if (action == "Update system") {
            args << "pacman" << "-Syyu";
        } else if (action == "Reinstall grub") {
            args << "bash" << "-c"
                 << "grub-install --recheck && grub-mkconfig -o /boot/grub/grub.cfg";
        } else if (action == "Install LTS kernel") {
            args << "pacman" << "-S" << "linux-lts";
        } else if (action == "Add new administrator") {
            bool ok = false;
            QString user = QInputDialog::getText(this, "Add User", "Username", QLineEdit::Normal, "", &ok);
            if (!ok || user.isEmpty()) return;
            args << "bash" << "-c" << QString("useradd -m -G wheel %1 && passwd %1").arg(user);
        }

        if (args.size() < 2) return; // nothing to run

        terminal->clear();
        process->setProcessChannelMode(QProcess::MergedChannels);
        process->start(program, args);
    }
    void appendOutput() {
        terminal->append(QString::fromLocal8Bit(process->readAll()));
    }
    void processFinished() {
        terminal->append("Finished.");
    }
    void cancelAction() {
        process->kill();
    }
private:
    void createPartitionPage() {
        partitionPage = new QWidget();
        QVBoxLayout *layout = new QVBoxLayout(partitionPage);
        partitionList = new QListWidget();
        QPushButton *selectBtn = new QPushButton("Select");
        layout->addWidget(new QLabel("Choose partition to chroot into:"));
        layout->addWidget(partitionList);
        layout->addWidget(selectBtn);
        connect(selectBtn, &QPushButton::clicked, this, &RecoveryWindow::selectPartition);
        stacked->addWidget(partitionPage);
        populatePartitions();
    }
    void createActionPage() {
        actionPage = new QWidget();
        QHBoxLayout *layout = new QHBoxLayout(actionPage);
        actionList = new QListWidget();
        actionList->addItems({"Update system", "Reinstall grub", "Install LTS kernel", "Add new administrator"});
        QWidget *right = new QWidget();
        QVBoxLayout *rightLayout = new QVBoxLayout(right);
        QHBoxLayout *buttons = new QHBoxLayout();
        QPushButton *runBtn = new QPushButton("Run");
        QPushButton *cancelBtn = new QPushButton("Cancel");
        buttons->addWidget(runBtn);
        buttons->addWidget(cancelBtn);
        terminal = new QTextEdit();
        terminal->setReadOnly(true);
        QFont f("monospace");
        f.setStyleHint(QFont::TypeWriter);
        terminal->setFont(f);
        rightLayout->addLayout(buttons);
        rightLayout->addWidget(terminal);
        layout->addWidget(actionList);
        layout->addWidget(right);
        connect(runBtn, &QPushButton::clicked, this, &RecoveryWindow::runAction);
        connect(cancelBtn, &QPushButton::clicked, this, &RecoveryWindow::cancelAction);
        process = new QProcess(this);
        connect(process, &QProcess::readyRead, this, &RecoveryWindow::appendOutput);
        connect(process, QOverload<int,QProcess::ExitStatus>::of(&QProcess::finished), this, &RecoveryWindow::processFinished);
        stacked->addWidget(actionPage);
    }
    void populatePartitions() {
        QProcess lsblk;
        lsblk.start("lsblk -ln -o NAME");
        lsblk.waitForFinished();
        const QString output = lsblk.readAllStandardOutput();
        for(const QString &line : output.split('\n')) {
            if(line.isEmpty()) continue;
            new QListWidgetItem("/dev/" + line.trimmed(), partitionList);
        }
    }
    QStackedWidget *stacked;
    QWidget *partitionPage;
    QWidget *actionPage;
    QListWidget *partitionList;
    QListWidget *actionList;
    QTextEdit *terminal;
    QString targetRoot;
    QProcess *process;
};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    RecoveryWindow w;
    w.show();
    return app.exec();
}

#include "main.moc"
