#ifndef IPRANGESELECTIONFORM_H
#define IPRANGESELECTIONFORM_H

#include <QtGui/QWidget>
#include <QtGui/QSpinBox>
#include <QtGui/QLineEdit>
#include "ui_ipRangeSelectionForm.h"
#include "HoneydConfiguration.h"

class MaskSpinBox : public QSpinBox
{
	Q_OBJECT

public:

	MaskSpinBox(QWidget * parent = 0) : QSpinBox(parent)
	{
		this->lineEdit()->setReadOnly(true);
		setWrapping(true);
		setRange(0, 32);
		setValue(24);
	}

protected:
	int valueFromText(const QString &text) const
	{
		QString temp = QString(text);
		in_addr_t mask = ntohl(inet_addr(temp.toStdString().c_str()));
		return Nova::HoneydConfiguration::GetMaskBits(mask);
	}

	QString textFromValue(int value) const
	{
		in_addr_t mask = 0;
		int i;
		for(i = 0; i < value; i++)
		{
			mask++;
			mask = mask << 1;
		}
		mask = mask << (31-i);
		in_addr temp;
		temp.s_addr = ntohl(mask);
		return QString(inet_ntoa(temp));
	}
	QValidator::State validate(QString &text) const
	{
		if(this->hasFocus())
			return QValidator::Intermediate;
		else if((inet_addr(text.toStdString().c_str()) == INADDR_NONE))
			return QValidator::Invalid;
		return QValidator::Acceptable;
	}
};

class ipRangeSelectionForm : public QWidget
{
    Q_OBJECT

public:
    ipRangeSelectionForm(QWidget *parent = 0);
    ~ipRangeSelectionForm();

private:
    Ui::ipRangeSelectionFormClass ui;

    QButtonGroup *m_interfaceCheckBoxes;
};

#endif // IPRANGESELECTIONFORM_H
