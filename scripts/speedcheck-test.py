from selenium import webdriver
from selenium.webdriver.common.by import By
from selenium.webdriver.firefox.options import Options

options = webdriver.FirefoxOptions()

options.add_argument("--headless")

driver = webdriver.Firefox(options=options)

driver.get("https://www.speedcheck.org")

driver.implicitly_wait(4)

start_button = driver.find_element(By.CLASS_NAME, "start-button")

start_button.click()

driver.implicitly_wait(50)

labels = driver.find_elements(By.CLASS_NAME, "cell-label")
elements = driver.find_elements(By.CLASS_NAME, "cell-value")

print(labels[1].text, elements[1].text)
print(labels[3].text, elements[3].text)


driver.quit()

