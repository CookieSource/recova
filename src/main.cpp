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
        if (!partitionList->currentItem()) return;
        targetRoot = partitionList->currentItem()->data(Qt::UserRole).toString();
        if (targetRoot.isEmpty()) return;
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
        } else if (action == "Install NVIDIA DKMS drivers") {
            args << "pacman" << "-S" << "nvidia-dkms";
        } else if (action == "Blacklist NVIDIA drivers") {
            args << "bash" << "-c" << "echo 'blacklist nvidia' > /etc/modprobe.d/blacklist-nvidia.conf";
        } else if (action == "Install AMD GPU drivers") {
            args << "pacman" << "-S" << "xf86-video-amdgpu";
        } else if (action == "Blacklist AMD GPU drivers") {
            args << "bash" << "-c" << "echo 'blacklist amdgpu' > /etc/modprobe.d/blacklist-amdgpu.conf";
        } else if (action == "Install NVIDIA open source drivers") {
            args << "pacman" << "-S" << "nvidia-open-dkms";
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
        partitionTerminal = new QTextEdit();
        partitionTerminal->setReadOnly(true);
        QFont f("monospace");
        f.setStyleHint(QFont::TypeWriter);
        partitionTerminal->setFont(f);
        layout->addWidget(new QLabel("Available chroot targets:"));
        layout->addWidget(partitionList);
        layout->addWidget(partitionTerminal);
        layout->addWidget(selectBtn);
        connect(selectBtn, &QPushButton::clicked, this, &RecoveryWindow::selectPartition);
        stacked->addWidget(partitionPage);
        populatePartitions();
    }
    void createActionPage() {
        actionPage = new QWidget();
        QHBoxLayout *layout = new QHBoxLayout(actionPage);
        actionList = new QListWidget();
        actionList->addItems({
            "Update system",
            "Reinstall grub",
            "Install LTS kernel",
            "Add new administrator",
            "Install NVIDIA DKMS drivers",
            "Blacklist NVIDIA drivers",
            "Install AMD GPU drivers",
            "Blacklist AMD GPU drivers",
            "Install NVIDIA open source drivers"});
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
        partitionList->clear();
        if (partitionTerminal)
            partitionTerminal->clear();

        QProcess lsblk;
        lsblk.start("lsblk -ln -o NAME,MOUNTPOINT");
        lsblk.waitForFinished();
        const QString output = QString::fromLocal8Bit(lsblk.readAllStandardOutput());
        for (const QString &line : output.split('\n')) {
            if (line.trimmed().isEmpty())
                continue;
            QStringList fields = line.simplified().split(' ');
            if (fields.size() < 2)
                continue;
            QString name = fields.at(0);
            QString mount = fields.at(1);
            QString tempMount;
            if (mount.isEmpty()) {
                tempMount = QString("/mnt/recova-%1").arg(name);
                QDir().mkpath(tempMount);
                if (partitionTerminal)
                    partitionTerminal->append(QString("Mounting /dev/%1 to %2...").arg(name, tempMount));
                QProcess mnt;
                mnt.setProcessChannelMode(QProcess::MergedChannels);
                mnt.start("mount", QStringList{QString("/dev/%1").arg(name), tempMount});
                mnt.waitForFinished();
                if (partitionTerminal)
                    partitionTerminal->append(QString::fromLocal8Bit(mnt.readAll()));
                if (mnt.exitStatus() != QProcess::NormalExit || mnt.exitCode() != 0) {
                    if (partitionTerminal)
                        partitionTerminal->append("Failed to mount\n");
                    QDir(tempMount).rmdir(".");
                    continue;
                }
                mount = tempMount;
            }
            if (partitionTerminal)
                partitionTerminal->append(QString("Testing %1 mounted at %2...").arg(name, mount));

            QProcess test;
            test.setProcessChannelMode(QProcess::MergedChannels);
            test.start("arch-chroot", QStringList{mount, "/bin/true"});
            test.waitForFinished();
            if (partitionTerminal)
                partitionTerminal->append(QString::fromLocal8Bit(test.readAll()));
            if (test.exitStatus() == QProcess::NormalExit && test.exitCode() == 0) {
                if (partitionTerminal)
                    partitionTerminal->append("OK\n");
                QListWidgetItem *it = new QListWidgetItem(QString("%1 (%2)").arg(name, mount), partitionList);
                it->setData(Qt::UserRole, mount);
            } else {
                if (partitionTerminal)
                    partitionTerminal->append("FAILED\n");
            }
            if (!tempMount.isEmpty()) {
                QProcess umnt;
                umnt.start("umount", QStringList{tempMount});
                umnt.waitForFinished();
                QDir(tempMount).rmdir(".");
            }
            qApp->processEvents();
        }
        if (partitionList->count() == 0 && partitionTerminal)
            partitionTerminal->append("No valid chroot targets found.");
    }
    QStackedWidget *stacked;
    QWidget *partitionPage;
    QWidget *actionPage;
    QListWidget *partitionList;
    QListWidget *actionList;
    QTextEdit *terminal;
    QTextEdit *partitionTerminal;
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
