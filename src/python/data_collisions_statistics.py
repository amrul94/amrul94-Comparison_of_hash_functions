import json
import os.path

import matplotlib.pyplot as plt
import matplotlib.ticker as ticker

from helper import *


class DataCollisionsStatistics(CollisionsStatistics):
    def __init__(self, tests_data: dict, tests_dir_name: str):
        CollisionsStatistics.__init__(self, tests_data)
        self.tests_dir_path = get_python_report_path(tests_dir_name)
        self.test_name = tests_data["Test name"]
        self.hist_path = os.path.join(self.tests_dir_path, self.test_name)
        make_dir(self.tests_dir_path)
        make_dir(self.hist_path)

    def __auto_label(self, ax, rects):
        for rect in rects:
            height = rect.get_height()
            if self.bits == 16:
                ax.text(rect.get_x() + rect.get_width() / 2., 1.05 * height,
                        height, ha='center', va='bottom', rotation=15)
            else:
                ax.text(rect.get_x() + rect.get_width() / 2., 1.05 * height,
                        height, ha='center', va='bottom')

    def __bar(self, ax):
        x = self.collisions.keys()
        height = self.collisions.values()
        rects = plt.bar(x, height, color='c', edgecolor='b', linewidth=1)
        plt.xticks(rotation='vertical')
        self.__auto_label(ax, rects)

        max_height = max([rect.get_height() for rect in rects])
        ax.yaxis.set_major_locator(ticker.MultipleLocator(max_height / 5))
        ax.yaxis.set_minor_locator(ticker.MultipleLocator(max_height / 25))
        plt.grid(ls=':')
        plt.ylim(top=max_height*1.2)

    def create_histogram(self, file_name):
        fig, ax = plt.subplots()
        ax.set_title(f'{self.bits} bits hashes')
        plt.xlabel('Hash name')
        plt.ylabel('Collisions')

        self.__bar(ax)

        file_path = os.path.join(self.hist_path, f'{self.bits} bits.png')
        fig.savefig(file_path, bbox_inches='tight')


def create_histogram(tests_dir_name: str, dir_path: str, file_name: str | bytes):
    path_to_file = os.path.join(dir_path, file_name)
    with open(path_to_file, 'r') as file:
        js = json.load(file)
        ews = DataCollisionsStatistics(js, tests_dir_name)
        ews.create_histogram(file_name)


def process_collision_statistics(tests_dir_name, test_name):
    path_to_tests_dir = get_cpp_report_path(tests_dir_name)
    path_to_cur_test_dir = os.path.join(path_to_tests_dir, test_name)
    list_of_files = os.listdir(path_to_cur_test_dir)
    for file_name in list_of_files:
        create_histogram(tests_dir_name, path_to_cur_test_dir, file_name)


