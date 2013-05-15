#include <QFileDialog>
#include <QPointer>

#include "qtmeegofilepicker.h"

typedef QStringList (*_qt_file_open_filenames_hook)
 (QWidget * parent, const QString &caption, const QString &dir,
  const QString &filter, QString *selectedFilter, QFileDialog::Options options);

typedef QString (*_qt_file_open_filename_hook)
 (QWidget * parent, const QString &caption, const QString &dir,
  const QString &filter, QString *selectedFilter, QFileDialog::Options options);

typedef QString (*_qt_file_save_filename_hook)
 (QWidget * parent, const QString &caption, const QString &dir,
  const QString &filter, QString *selectedFilter, QFileDialog::Options options);

typedef QString (*_qt_file_existing_directory_hook)
 (QWidget *parent, const QString &caption, const QString &dir,
  QFileDialog::Options options);

Q_GUI_EXPORT _qt_file_open_filename_hook qt_filedialog_open_filename_hook;
Q_GUI_EXPORT _qt_file_open_filenames_hook qt_filedialog_open_filenames_hook;
Q_GUI_EXPORT _qt_file_save_filename_hook qt_filedialog_save_filename_hook;
Q_GUI_EXPORT _qt_file_existing_directory_hook qt_filedialog_existing_directory_hook;


class QtMeegoFilePickerHooks {

  public:
    QtMeegoFilePickerHooks()
    {
        qt_filedialog_open_filename_hook = &openFilename;
        qt_filedialog_save_filename_hook = &saveFilename;
        qt_filedialog_open_filenames_hook = &openFilenames;
        qt_filedialog_existing_directory_hook = &openDirectory;
    }

    static QString openFilename(QWidget *parent, const QString &caption,
                                const QString &dir, const QString &filter,
                                QString *selectedFilter,
                                QFileDialog::Options options)
    {
        Q_UNUSED( caption )
        Q_UNUSED( dir )
        Q_UNUSED( filter )
        Q_UNUSED( selectedFilter )
        Q_UNUSED( options )

        QPointer<QtMeegoFilePicker> picker = new QtMeegoFilePicker(parent);
        picker->setMode(QtMeegoFilePicker::Mode_OpenFile);
        picker->exec();

        QString selectedFile;
        if(picker) {
            selectedFile = picker->getSelectedFileName();
            delete picker;
        }
	return selectedFile;
    }

    static QStringList openFilenames(QWidget *parent, const QString &caption,
                                    const QString &dir, const QString &filter,
                                    QString *selectedFilter,
                                    QFileDialog::Options options)
    {
        Q_UNUSED( caption )
        Q_UNUSED( dir )
        Q_UNUSED( filter )
        Q_UNUSED( selectedFilter )
        Q_UNUSED( options )

        QPointer<QtMeegoFilePicker> picker = new QtMeegoFilePicker(parent);
        picker->setMode(QtMeegoFilePicker::Mode_OpenFiles);
        picker->exec();

        QStringList selectedFileNames;
        if(picker) {
            selectedFileNames = picker->getSelectedFileNames();
            delete picker;
        }
        return selectedFileNames;
    }

    static QString openDirectory(QWidget *parent, const QString &caption, const
                                QString &dir, QFileDialog::Options options)
    {
        Q_UNUSED( caption )
        Q_UNUSED( dir )
        Q_UNUSED( options )

        QPointer<QtMeegoFilePicker> picker = new QtMeegoFilePicker(parent);
        picker->setMode(QtMeegoFilePicker::Mode_OpenDirectory);
        picker->exec();
        if(picker)
            delete picker;

        return "";
    }

    static QString saveFilename(QWidget *parent, const QString &caption,
                                const QString &dir, const QString &filter,
                                QString *selectedFilter,
                                QFileDialog::Options options)
    {
        Q_UNUSED( caption )
        Q_UNUSED( dir )
        Q_UNUSED( filter )
        Q_UNUSED( selectedFilter )
        Q_UNUSED( options )

        QPointer<QtMeegoFilePicker> picker = new QtMeegoFilePicker(parent);
        picker->setMode(QtMeegoFilePicker::Mode_SaveFile);
        picker->exec();
        if(picker)
            delete picker;

        return "";
    }
};

static QtMeegoFilePickerHooks filePickerHooks;
