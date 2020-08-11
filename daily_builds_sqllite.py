import datetime
from covid import *
import sqlite3


def r0_range(start, stop):
    r0 = []
    for i in range(start, stop):
        if i == 0:
            r0.append(i)
        r0.append(float(i + 0.25))
        r0.append(float(i + .5))
        r0.append(float(i + .75))
        r0.append(float(i + 1))
    print('r0_range = ' + str(r0))
    return r0


def find_para_id(r0_opt, max_dur_opt, mult_opt, conn, c):
    day_zero = datetime.date(2020, 2, 29)
    # begins a read transaction, or a write transaction if someone else is writing
    c.execute(
        """SELECT * FROM projection_parameter WHERE
            r0_value =:r0_value AND
            max_dur =:max_dur AND
            mult =:mult""", {
            "r0_value": r0_opt,
            "max_dur": max_dur_opt,
            "mult": mult_opt
        })
    parameter_search = c.fetchall()
    if len(parameter_search) != 0:
        this_p = parameter_search[0]
        return this_p
    else:
        # begins a write transaction
        c.execute(
            """INSERT INTO projection_parameter(
                r0_value,
                max_dur,
                cont_dur,
                mult,
                date_updated,
                time_updated,
                day_zero)
            VALUES(?, ?, ?, ?, null, null, ?);""",
            (r0_opt, max_dur_opt, 7, mult_opt, day_zero))
        conn.commit()
        # begins a read transaction
        c.execute(
            """SELECT * FROM projection_parameter WHERE
            r0_value =:r0_value AND
            max_dur =:max_dur AND
            mult =:mult""", {
                "r0_value": r0_opt,
                "max_dur": max_dur_opt,
                "mult": mult_opt
            })
        this_p = c.fetchone()
        print('created new model with id of ' + str(this_p['id']))
        return this_p['id']


def find_output(this_p_id, c):
    # begins a read transaction
    c.execute(
        "SELECT * FROM projection_output WHERE paramID_id =:paramID_id ORDER BY _day_since_zero ASC",
        {'paramID_id': str(this_p_id)})
    output_search = c.fetchall()
    return output_search
    # try c.rowcount


def update_model(r0_opt, max_dur_opt, mult_opt, conn, c):
    this_p_id = find_para_id(r0_opt, max_dur_opt, mult_opt, conn, c)
    output = find_output(this_p_id, c)
    if len(output) != 0:
        print('found' + str(len(output)) + 'output entries')
    # pass local model results to output objects
    new_model = Model_30_set(r0_opt, max_dur_opt, mult_opt)
    new_model_size = Model_30_get_size(new_model)
    updated_output_count = 0
    new_output_count = 0
    for i in range(new_model_size):
        days_since = datetime.timedelta(days=i)
        day_zero = datetime.date(2020, 2, 29)
        this_date = day_zero + days_since
        # if there is prev output object, update
        if i <= len(output) - 1:
            # python sqlite3 module issues a BEGIN statement here implicitly
            c.execute(
                """UPDATE projection_output
                    SET _new_cases = :_new_cases,
                    _date_updated = :_date_updated
                    WHERE paramID_id = :paramID_id
                    AND _day_since_zero = :_day_since_zero""",
                {
                    "paramID_id": str(this_p_id),
                    "_day_since_zero": str(i),
                    "_new_cases": str(Model_30_get_cases(new_model, i)),
                    "_date_updated": str(datetime.date.today())
                })
            updated_output_count += 1
        # otherwise create new output object
        else:
            # python sqlite3 module issues a BEGIN statement here implicitly
            c.execute(
                """INSERT INTO projection_output(
                paramID_id,
                _day,
                _day_since_zero,
                _date_updated,
                _new_cases,
                _new_deaths)
            VALUES(?, ?, ?, ?, ?, null);""",
                (str(this_p_id), str(this_date), str(i),
                 str(datetime.date.today()),
                 str(Model_30_get_cases(new_model, i))))
            new_output_count += 1
    # update date
    # print('updated ' + str(updated_output_count) + ' output entries')
    print('created ' + str(new_output_count) + ' new output entries')
    # python sqlite3 module issues a BEGIN statement here implicitly
    c.execute(
                """UPDATE projection_parameter
                    SET date_updated = :date_updated,
                    time_updated = :time_updated,
                    model_size = :model_size
                    WHERE id = :id""",
                {
                    "id": str(this_p_id),
                    "date_updated": str(datetime.date.today()),
                    "time_updated": str(datetime.datetime.now().time),
                    "model_size": str(new_model_size)
                })
    conn.commit()
    # print('date updated now ' + str(datetime.date.today()))
    print('{} saved'.format(this_p_id))


def update_models(r0_list, max_dur_list, mult_list):
    # update this to be multithreaded
    update_model_count = 0
    for r0_opt in r0_list:
        for max_dur_opt in max_dur_list:
            for mult_opt in mult_list:
                # modify isolation_level parameter to change BEGIN statements: DEFERRED, IMMEDIATE OR EXCLUSIVE
                conn = sqlite3.connect(
                    "/home/anna/code/exercises/flask/covid_model/db.sqlite3", isolation_level="DEFERRED")
                conn.row_factory = sqlite3.Row
                c = conn.cursor()
                eliminate_duplicate_models(r0_opt, max_dur_opt, mult_opt, conn, c)
                update_model(r0_opt, max_dur_opt, mult_opt, conn, c)
                print(str(conn.total_changes) + ' entries have been changed')
                conn.close()
                update_model_count += 1
    print(str(update_model_count) + ' models have been created/updated')


def eliminate_duplicate_models(r0_opt, max_dur_opt, mult_opt, conn, c):
    # begins read transaction
    c.execute(
        """SELECT * FROM projection_parameter WHERE
            r0_value =:r0_value AND
            max_dur =:max_dur AND
            mult =:mult""", {
            "r0_value": r0_opt,
            "max_dur": max_dur_opt,
            "mult": mult_opt
        })
    parameter_search = c.fetchall()
    to_delete = []
    duplicates = 0
    if len(parameter_search) > 1:
        for i in range(1, len(parameter_search)):
            to_delete.append(parameter_search[i]['id'])
    for row_id in to_delete:
        conn.execute("DELETE from projection_parameter WHERE id=?", row_id)
        duplicates += 1
    print('eliminated ' + str(duplicates) + ' duplicates')


def main():
    r0 = r0_range(0, 6)
    max_dur = [30, 45]
    mult = [0, 1, 2, 3]
    update_models(r0, max_dur, mult)


main()

