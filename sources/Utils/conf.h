/**
 * @file: conf.h
 * Implementation of internal configuration of ShowGraph
 */
/*
 * Utils library in Showgraph tool
 * Copyright (C) 2009  Boris Shurygin
 */
#ifndef CONF_H
#define CONF_H

#include <QString>
#include <QHash>

/**
 * Command line option type
 */
enum OptType
{
    /** Boolean */
    OPT_BOOL,
    /** Integer */
    OPT_INT,
    /** Float */
    OPT_FLOAT,
    /** String */
    OPT_STRING,
    /** Number of types */
    OPT_TYPES_NUM
};

/**
 * Union type for option's values
 */
union OptValues
{
    /** Value for boolean option */
    bool bool_val;
    /** Value for integer option */
    int int_val;
    /** Value for floating point option */
    qreal float_val;
}; 

/**
 * Command line option descrition
 */
class Option
{
    /** Type */
    OptType _type;

    /** Option default values */
    OptValues def_values;

    /** Option user's values */
    OptValues values;

    QString string_val;
    
    /** Short name */
    QString short_name;
    
    /** Long name */
    QString long_name;

    /** Description */
    QString descr;
public:
    /** Constructor without default val */
    Option( OptType _t, QString sname, QString lname, QString d):
        _type( _t), short_name( sname), long_name( lname), descr( d)
    {
        switch ( _t)
        {   
            case OPT_BOOL:
                def_values.bool_val = false;
                break;
            case OPT_INT:
                def_values.int_val = 0;
                break;
            case OPT_FLOAT:
                def_values.float_val = 0;
                break;
            case OPT_STRING:
            default:
                break;
        }
        values = def_values;
    };

    /** Constructor with default bool val */
    Option( QString sname, QString lname, QString d, bool val):
        _type( OPT_BOOL), short_name( sname), long_name( lname), descr( d)
    {
        def_values.bool_val = val;
        values = def_values;
    }

    /** Constructor for string option */
    Option( QString sname, QString lname, QString d):
        _type( OPT_STRING), short_name( sname), long_name( lname), descr( d){};

    /** Get short name */
    inline QString shortName() const
    {
        return short_name;    
    }
    /** Get long name */
    inline QString longName() const
    {
        return long_name;    
    }
    /** Get option type */
    inline OptType type() const
    {
        return _type;
    }
    /** Get option default val */
    inline bool defBoolVal() const
    {
        assertd( _type == OPT_BOOL);
        return def_values.bool_val;
    }
    /** Set option boolean value */
    inline void setBoolVal( bool val)
    {
        assertd( _type == OPT_BOOL);
        values.bool_val = val;
    }
        /** Set option boolean value */
    inline void setIntVal( int val)
    {
        assertd( _type == OPT_INT);
        values.int_val = val;
    }
        /** Set option boolean value */
    inline void setFloatVal( qreal val)
    {
        assertd( _type == OPT_FLOAT);
        values.float_val = val;
    }
        /** Set option boolean value */
    inline void setStringVal( QString val)
    {
        assertd( _type == OPT_STRING);
        string_val = val;
    }
    /** Get string value of option */
    inline QString string() const
    {
        assertd( _type == OPT_STRING);
        return string_val;
    }
    /** Get int value of option */
    inline int intVal() const
    {
        assertd( _type == OPT_INT);
        return values.int_val;
    }
    /** Get float value of option */
    inline qreal floatVal() const
    {
        assertd( _type == OPT_FLOAT);
        return values.float_val;
    }
    /** Get int value of option */
    inline int isSet() const
    {
        assertd( _type == OPT_BOOL);
        return values.bool_val;
    }
    /** Print option's synopsis and description */
    void print();
};

/**
 * Configuration class
 */
class Conf
{
    QString app_name;
    QHash< QString, Option *> short_opts;
    QHash< QString, Option *> long_opts;

    QList< QString> unknown_options;
public:
    /** Constructor */
    Conf();
    
    /** Destructor wipes all the options */
    ~Conf()
    {
        foreach( Option *opt, short_opts)
        {
            delete opt;
        }
    }
    /** Get number of arguments that were not recognized */
    inline int unknownOptsNum() const
    {
        return unknown_options.count();
    }
    /** Add option */
    inline void addOption( Option *opt)
    {
        short_opts[ opt->shortName()] = opt;
        long_opts[ opt->longName()] = opt;
    }

    /** Convenience routine: adds option without default val */
    inline void addOption( OptType _t, QString sname, QString lname, QString d)
    {
        Option *opt = new Option( _t, sname, lname, d);
        addOption( opt);
    }

    /** Convenience routine: adds option with default bool val */
    inline void addOption( QString sname, QString lname, QString d, bool val)
    {
        Option *opt = new Option( sname, lname, d, val);
        addOption( opt);
    }
    /** Convenience routine: adds string option without default value */
    inline void addOption( QString sname, QString lname, QString d)
    {
        Option *opt = new Option( sname, lname, d);
        addOption( opt);
    }
    
    /** Print options */
    void printOpts();

    /** Print value defaults */
    void printDefaults();

    /** Parse args */
    void readArgs( int argc, char** argv);

    /** Get option based on its name */
    inline Option* option( QString name)
    {
        /* try to look among short options */
        if ( short_opts.find( name) != short_opts.end())
        {
            return short_opts[ name];
        }
        /* try to look among long options */
        if ( long_opts.find( name) != long_opts.end())
        {
            return long_opts[ name];
        }
        /* if nothing's found return NULL */
        return NULL;
    }
    /** Get option based on its short name */
    inline Option* shortOption( QString name)
    {
        /* try to look among short options */
        if ( short_opts.find( name) != short_opts.end())
        {
            return short_opts[ name];
        }
        /* if nothing's found return NULL */
        return NULL;
    }
    /** Get option based on its long name */
    inline Option* longOption( QString name)
    {
        /* try to look among long options */
        if ( long_opts.find( name) != long_opts.end())
        {
            return long_opts[ name];
        }
        /* if nothing's found return NULL */
        return NULL;
    }
};

#endif