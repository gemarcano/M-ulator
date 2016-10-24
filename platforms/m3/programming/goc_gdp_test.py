#!/usr/bin/env python2

import code
try:
	import Image
except ImportError:
	from PIL import Image

import gdp

gdp.gdp_init()
gcl_name = gdp.GDP_NAME("edu.umich.eecs.m3.test01")
gcl_handle = gdp.GDP_GCL(gcl_name, gdp.GDP_MODE_RA)

#j = Image.open('/tmp/capture1060.jpeg')
#d = {"data": j.tostring()}

#gcl_handle.append(d)

record = gcl_handle.read(3)
raw = record['data']

image = Image.fromstring('RGB', (640,640), raw)
image.show()

#code.interact(local=locals())