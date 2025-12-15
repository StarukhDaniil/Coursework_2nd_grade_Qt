#ifndef GRAPHSESSION_H
#define GRAPHSESSION_H

#include <QVector>
#include <QPointF>
#include <QString>

class GraphSession
{
public:
    explicit GraphSession(const QString &name);

    void appendData(double x, double y);

    const QString& getName() const { return m_name; }
    const QVector<QPointF>& getData() const { return m_data; }

private:
    QString m_name;
    QVector<QPointF> m_data;
};

#endif // GRAPHSESSION_H
