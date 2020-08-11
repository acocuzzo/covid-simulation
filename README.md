Covid simulation in CPP and wrapper in C, modeling cases per day based on NYC data, population size, age-distribution, and mortality rates by age.
Created python module using swig
Cron Jobs:
Web-page scraper for NYC daily case data (we exclude the last four days of data as these numbers are delayed by four days).
Daily builds - Run the model nightly based on updated data at 9:30 pm, update sqlite database
