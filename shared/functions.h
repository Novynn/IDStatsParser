#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include <QByteArray>

class Functions
{
public:
    static QByteArray encodeStatString(QByteArray data){
        unsigned char mask = 1;
        QByteArray result;

        for (int i = 0; i < data.size(); ++i ){
            if ((data.at(i) % 2) == 0)
                result.append( data.at(i) + 1 );
            else {
                result.append(data[i]);
                mask |= 1 << ( ( i % 7 ) + 1 );
            }
            if (i % 7 == 6 || i == data.size() - 1){
                result.insert( result.length() - 1 - ( i % 7 ), mask);

                mask = 1;
            }
        }

        return result;
    }

    static QByteArray decodeStatString(QByteArray data){
        QByteArray result;

        uchar mask = 1;
        for (int i = 0; i < data.size(); i++){
            if ((i % 8) == 0)
                mask = (uchar)data[i];
            else{
                uchar val;
                if (( mask & (0x1 << (i % 8))) == 0)
                    val = (uchar)data[i] - 1;
                else
                    val = (uchar)data[i];
                result.append(val);
            }
        }

        return result;
    }
};

#endif // FUNCTIONS_H
