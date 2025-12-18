#include "graphsession.h"

GraphSession::GraphSession(const QString &name)
    : m_name(name)
{
}

void GraphSession::appendData(double x, double y)
{
    m_data.append(QPointF(x, y));
}
