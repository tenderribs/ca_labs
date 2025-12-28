# python3 -m pip install jinja2

from jinja2 import Environment, FileSystemLoader
env = Environment(loader = FileSystemLoader('report/yml_configs'))

template = env.get_template('lab5_t4_templ.jinja')

schedulers = ['FCFS', 'FRFCFS', 'ATLAS', 'BLISS']
core_configs = [(1, 3), (2, 2), (0, 4), (0, 8), (0, 1), (1, 0)] # (num_l, num_h)

# Create the multi core YAML files
for scheduler in schedulers:
    for core_config in core_configs:
        num_l, num_h = core_config

        # create string from the template
        content = template.render(
            num_l=num_l,
            num_h=num_h,
            scheduler=scheduler
        )

        fname = f"{scheduler}_{num_l}L{num_h}H.yaml"
        print(fname)

        # write the string into a yaml file
        with open(f"report/yml_configs/all/{fname}", mode="w", encoding="utf-8") as results:
            results.write(content)