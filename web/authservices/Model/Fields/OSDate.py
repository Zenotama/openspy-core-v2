from peewee import *
import datetime
class OSDate(Field):
	db_field = 'date'
	def db_value(self, value):
		return datetime.date(value['year'], value['month'], value['day'])
	def python_value(self, value):
		return {'day': value.day, 'month': value.month, 'year': value.year}