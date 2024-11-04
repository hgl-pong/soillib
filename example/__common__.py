import os
import soillib as soil
import matplotlib.pyplot as plt
from matplotlib import colors

def iter_tiff(path):

  '''
  Generator for all Files in 
  Directory, or a Single File
  '''

  path = os.fsencode(path)
  if not os.path.exists(path):
    raise RuntimeError("path does not exist")

  if os.path.isfile(path):
    file = os.path.basename(path)
    yield file.decode('utf-8'), path.decode('utf-8')

  elif os.path.isdir(path):
    for file in os.listdir(path):
      yield file.decode('utf-8'), os.path.join(path, file).decode('utf-8')

  else:
    raise RuntimeError("path must be file or directory")

def relief_shade(h, n):

  # Regularize Height
  h_min = np.nanmin(h)
  h_max = np.nanmax(h)
  h = (h - h_min)/(h_max - h_min)

  # Light Direction, Diffuse Lighting
  light = np.array([ 1, 1, 2])
  light = light / np.linalg.norm(light)

  diffuse = np.sum(light * n, axis=-1)
  diffuse = 0.05 + 0.9*diffuse

  # Flat-Toning
  flattone = np.full(h.shape, 0.9)
  weight = 1.0 - n[:,:,2]
  weight = weight * (1.0 - h * h)

  # Full Diffuse Shading Value
  diffuse = (1.0 - weight) * diffuse + weight * flattone
  return diffuse

'''
Specialized Plotting Functions
'''

def plot_area(area):

  fig, ax = plt.subplots(figsize=(8,6))
  fig.patch.set_alpha(0)
  plt.grid('on', zorder=0)
  im = ax.imshow(area, zorder=2,
                cmap='CMRmap',
                norm=colors.LogNorm(1, area.max()),
                interpolation='bilinear')
  plt.colorbar(im, ax=ax, label='Upstream Cells')
#  plt.title('Flow Accumulation', size=14)
#  plt.xlabel('Longitude')
#  plt.ylabel('Latitude')
  plt.tight_layout()
  plt.show()

def plot_dem(model):

  grid, dem = model

  fig, ax = plt.subplots(figsize=(8,6))
  fig.patch.set_alpha(0)
  plt.imshow(dem, extent=grid.extent, cmap='terrain', zorder=1)
  plt.colorbar(label='Elevation (m)')
  plt.grid(zorder=0)
  plt.title('Digital elevation map', size=14)
  plt.xlabel('Longitude')
  plt.ylabel('Latitude')
  plt.tight_layout()
  plt.show()

def plot_flow(model):

  grid, fdir, dirmap = model

  fig = plt.figure(figsize=(8,6))
  fig.patch.set_alpha(0)
  plt.imshow(fdir, cmap='viridis', zorder=2)
  boundaries = ([0] + sorted(list(dirmap)))
  plt.colorbar()#boundaries= boundaries,
              #values=sorted(dirmap))
  plt.xlabel('Longitude')
  plt.ylabel('Latitude')
  plt.title('Flow direction grid', size=14)
  plt.grid(zorder=-1)
  plt.tight_layout()
  plt.show()

def show_height(array, index):

  array = soil.cached(array)
  data = array.numpy(index)
  plt.imshow(data)
  plt.show()

def show_normal(array, index):

  array = soil.cached(array)
  normal = soil.normal(index, array)

  data = normal.full().numpy(index)
  plt.imshow(data)
  plt.show()

def show_relief(array, index):

  array = soil.cached(array)
  normal = soil.normal(index, array)

  normal_data = normal.full().numpy(index)
  height_data = array.numpy(index)
  
  relief = relief_shade(height_data, normal_data) 
  plt.imshow(relief, cmap='gray')
  plt.show()