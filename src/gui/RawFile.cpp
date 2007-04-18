/* 
 * $Id: RawFile.cpp,v 1.2 2006/08/11 19:58:07 srhea Exp $
 *
 * Copyright (c) 2006 Sean C. Rhea (srhea@srhea.net)
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 * 
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc., 51
 * Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "RawFile.h"
#include <math.h>
#include <assert.h>
#include <QMessageBox>
#include "srm.h"

extern "C" {
#include "pt.h"
}

struct RawFileReadState
{
    QList<RawFilePoint*> &points;
    QMap<double,double> &powerHist;
    QStringList &errors;
    double last_secs, last_miles;
    unsigned last_interval;
    time_t start_since_epoch;
    unsigned rec_int;
    RawFileReadState(QList<RawFilePoint*> &points, 
                     QMap<double,double> &powerHist,
                     QStringList &errors) : 
        points(points), powerHist(powerHist), errors(errors), last_secs(0.0), 
        last_miles(0.0), last_interval(0), start_since_epoch(0), rec_int(0) {}
};

static void 
config_cb(unsigned /*interval*/, unsigned rec_int, 
          unsigned /*wheel_sz_mm*/, void *context) 
{
    RawFileReadState *state = (RawFileReadState*) context;
    // Assume once set, rec_int should never change.
    assert((state->rec_int == 0) || (state->rec_int == rec_int));
    state->rec_int = rec_int;
}

static void
time_cb(struct tm *, time_t since_epoch, void *context)
{
    RawFileReadState *state = (RawFileReadState*) context;
    if (state->start_since_epoch == 0)
        state->start_since_epoch = since_epoch;
    double secs = since_epoch - state->start_since_epoch;
    RawFilePoint *point = new RawFilePoint(secs, -1.0, -1.0, -1.0, 
                                           state->last_miles, 0, 0, 
                                           state->last_interval);
    state->points.append(point);
    state->last_secs = secs;
}

static void
data_cb(double secs, double nm, double mph, double watts, double miles, 
        unsigned cad, unsigned hr, unsigned interval, void *context)
{
    RawFileReadState *state = (RawFileReadState*) context;
    RawFilePoint *point = new RawFilePoint(secs, nm, mph, watts, miles,
                                           cad, hr, interval);
    state->points.append(point);
    state->last_secs = secs;
    state->last_miles = miles;
    state->last_interval = interval;
    double sum = state->rec_int * 0.021;
    if (watts >= 0.0) {
        if (state->powerHist.contains(watts)) {
            sum += state->powerHist.value(watts);
            state->powerHist.remove(watts);
        }
        state->powerHist.insert(watts, sum);
    }
}

static void
error_cb(const char *msg, void *context) 
{
    RawFileReadState *state = (RawFileReadState*) context;
    state->errors.append(QString(msg));
}

RawFile *RawFile::readFile(QFile &file, QStringList &errors)
{
    RawFile *result = new RawFile(file.fileName());
    if (file.fileName().indexOf(".srm") == file.fileName().size() - 4) {
        SrmData data;
        if (!readSrmFile(file, data, errors)) {
            delete result;
            return NULL;
        }
        result->startTime = data.startTime;
        result->rec_int_ms = (unsigned) round(data.recint * 1000.0);

        QListIterator<SrmDataPoint*> i(data.dataPoints);
        while (i.hasNext()) {
            SrmDataPoint *p1 = i.next();
            RawFilePoint *p2 = new RawFilePoint(
                p1->secs, 0, p1->kph * 0.62137119, p1->watts,
                p1->km * 0.62137119, p1->cad, p1->hr, p1->interval);
            if (result->powerHist.contains(p2->watts))
                result->powerHist[p2->watts] += data.recint;
            else
                result->powerHist[p2->watts] = data.recint;
            result->points.append(p2);
        }
    }
    else {
        if (!result->file.open(QIODevice::ReadOnly)) {
            delete result;
            return NULL;
        }
        FILE *f = fdopen(result->file.handle(), "r");
        assert(f);
        RawFileReadState state(result->points, result->powerHist, errors);
        pt_read_raw(f, 0 /* not compat */, &state, config_cb, 
                    time_cb, data_cb, error_cb);
        result->rec_int_ms = (unsigned) round(state.rec_int * 1.26 * 1000.0);
    }
    return result;
}


