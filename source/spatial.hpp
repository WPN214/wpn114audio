#pragma once

#include <QObject>

//=================================================================================================
struct spatial_t
// TODO
//=================================================================================================
{
    qreal x = 0, y = 0, z = 0;
    qreal w = 0, d = 0, h = 0;
};

//=================================================================================================
class Spatial : public QObject
// TODO
//=================================================================================================
{
    Q_OBJECT

public:

    enum Alignment
    {
        Left        = 0,
        Centered    = 1,
        Right       = 2,
        Top         = 3,
        Bottom      = 4
    };

    Q_ENUM(Alignment)

    Spatial()
    {

    }

    Spatial(Spatial const&)
    {

    }

    Spatial& operator=(Spatial const&)
    {
        return *this;
    }

    bool operator!=(Spatial const&)
    {
        return false;
    }

private:
    std::vector<spatial_t>
    m_channels;
};

Q_DECLARE_METATYPE(Spatial)
