import requests
from bs4 import BeautifulSoup
import datetime


def scrape(url):
    resp = requests.get(url)
    if resp.status_code == 200:
        soup = BeautifulSoup(resp.text, 'html.parser')
        text = soup.get_text()
    return text


def write(text, path):
    f = open(path, "w")
    f.write(text)
    f.close()


def write_today_line():
    day_zero = datetime.date(2020, 2, 29)
    today = datetime.date.today()
    last_four_days = datetime.timedelta(days= 4)
    first_projected_day = today - last_four_days
    day_after_zero = first_projected_day - day_zero
    f = open("/home/anna/code/exercises/probability/projections/today_line.csv", "w")
    f.write("day,case\n")
    for i in range(0, 1000000):
        f.write(str(day_after_zero.days) + "," + str(i) + "\n")
    f.close()


def main():
    url_CHD = 'https://raw.githubusercontent.com/nychealth/coronavirus-data/master/case-hosp-death.csv'
    daily_CHD = scrape(url_CHD)
    write(daily_CHD, "/home/anna/code/exercises/probability/CHD.csv")

    url_rates_by_age = 'https://raw.githubusercontent.com/nychealth/coronavirus-data/master/by-age.csv'
    daily_rates_by_age = scrape(url_rates_by_age)
    write(daily_rates_by_age, "/home/anna/code/exercises/probability/rates_by_age.csv")

    url_rates_by_sex = 'https://raw.githubusercontent.com/nychealth/coronavirus-data/master/by-sex.csv'
    daily_rates_by_sex = scrape(url_rates_by_sex)
    write(daily_rates_by_sex, "/home/anna/code/exercises/probability/rates_by_sex.csv")

main()
