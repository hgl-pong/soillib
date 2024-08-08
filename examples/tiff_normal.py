#!/usr/bin/env python

import os
import soillib as soil
import matplotlib.pyplot as plt
import numpy as np

def main(path):

  path = os.fsencode(path)
  
  if not os.path.exists(path):
    raise RuntimeError("path does not exist")

  def show_tiff(path):

    img = soil.geotiff(path)

    # Get the Buffer Type
    height = img.buf()
    height.reshape(soil.shape([img.height, img.width]))

    normal = soil.normal(height)

    # Extract Data
    data = normal()
    data = np.array(data)
    # data = data.reshape((img.height, img.width, 3))
    print(data.shape, data.dtype)

    plt.imshow(data)
    plt.show()

  if os.path.isfile(path):

    show_tiff(path)

  elif os.path.isdir(path):
    for file in os.listdir(path):
      show_tiff(os.path.join(path, file))

if __name__ == "__main__":

  #data = "/home/nickmcdonald/Datasets/ViennaDGM/21_Floridsdorf"
  data = "/home/nickmcdonald/Datasets/UpperAustriaDGM/40718_DGM_tif_Traunkirchen"
  main(data)