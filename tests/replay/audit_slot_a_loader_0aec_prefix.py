#!/usr/bin/env python3
"""Actual loader prefix through first file-open; no file effects or bridge."""
import ctypes as ct
from hashlib import sha256
import json
import os
from pathlib import Path
import re
import sys
import time
import unicorn
ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tests/replay'))
import audit_slot_a_delete_abi_map as mapped
import audit_slot_a_string_0425_abi as resident
from audit_slot_a_string_0356_abi import CPU, State, Report, NAMES, cpu_values, compare_cpu
CASE_TIMEOUT = float(os.environ.get('POOLRAD_LOADER_CASE_TIMEOUT', '300'))
RAM_HASHES = {
    "19d7": "af272158b4330292fd40626b91b1c428a58059ecabab74bcd19c6af6c040f34d",
    "0aec": "94315d738a79183059f2002fae91205092c72bf30954f8c75c09c68ee1872028",
    "0aed": "13701c950fd6e6482c303efc7dd16064054936e609ea9539fb113ae8d71d1f00",
    "0aef": "13701c950fd6e6482c303efc7dd16064054936e609ea9539fb113ae8d71d1f00",
    "0af3": "13701c950fd6e6482c303efc7dd16064054936e609ea9539fb113ae8d71d1f00",
    "0af6": "13701c950fd6e6482c303efc7dd16064054936e609ea9539fb113ae8d71d1f00",
    "0af7": "166991413bca20fdaa43dc790442be0fd247c18d31769c359ab63e55b559a869",
    "0af8": "daecbb64769424e92da66ebf54453d2430d84809be441464df65c46ddef61180",
    "0afc": "daecbb64769424e92da66ebf54453d2430d84809be441464df65c46ddef61180",
    "0afd": "886b4d9493568a1f2d554645d8f1ed2e8356ae771469cf22db198fd6642103b7",
    "0afe": "a0802af36275af7b5857bb6b3299e5bad62d103229fa165f99c201137ca8a795",
    "0b01": "a0802af36275af7b5857bb6b3299e5bad62d103229fa165f99c201137ca8a795",
    "0b02": "a952100ef2765fd398896bab8734f01118ca4de016f473d1186f7ff6d4232420",
    "b02-0b25": "751cfe034edd3aefb4fbe00b5f75659776aaaf69a120d0979a89e9d0dfb3b514",
    "b02-0b26": "751cfe034edd3aefb4fbe00b5f75659776aaaf69a120d0979a89e9d0dfb3b514",
    "b02-0b28": "751cfe034edd3aefb4fbe00b5f75659776aaaf69a120d0979a89e9d0dfb3b514",
    "b02-0b2a": "751cfe034edd3aefb4fbe00b5f75659776aaaf69a120d0979a89e9d0dfb3b514",
    "b02-0b2e": "751cfe034edd3aefb4fbe00b5f75659776aaaf69a120d0979a89e9d0dfb3b514",
    "b02-0b32": "751cfe034edd3aefb4fbe00b5f75659776aaaf69a120d0979a89e9d0dfb3b514",
    "b02-0b36": "751cfe034edd3aefb4fbe00b5f75659776aaaf69a120d0979a89e9d0dfb3b514",
    "b02-0b37": "751cfe034edd3aefb4fbe00b5f75659776aaaf69a120d0979a89e9d0dfb3b514",
    "b02-0b39": "751cfe034edd3aefb4fbe00b5f75659776aaaf69a120d0979a89e9d0dfb3b514",
    "b02-0b3d": "751cfe034edd3aefb4fbe00b5f75659776aaaf69a120d0979a89e9d0dfb3b514",
    "b02-0b3e": "2431bc24ebe6dcdfe5a3aef2b8eb0a9abcfdb17745c4613f64f4d7ff7439a7be",
    "b02-0b40": "2431bc24ebe6dcdfe5a3aef2b8eb0a9abcfdb17745c4613f64f4d7ff7439a7be",
    "b02-0b42": "2431bc24ebe6dcdfe5a3aef2b8eb0a9abcfdb17745c4613f64f4d7ff7439a7be",
    "b02-0b44": "cdf02fb9e017ed1addd0aeda56336cfa6b15d73e81ff826cf3c348a4ae442b25",
    "b02-0b46": "cdf02fb9e017ed1addd0aeda56336cfa6b15d73e81ff826cf3c348a4ae442b25",
    "0b07": "cdf02fb9e017ed1addd0aeda56336cfa6b15d73e81ff826cf3c348a4ae442b25",
    "0b0b": "cdf02fb9e017ed1addd0aeda56336cfa6b15d73e81ff826cf3c348a4ae442b25",
    "0b0d": "cdf02fb9e017ed1addd0aeda56336cfa6b15d73e81ff826cf3c348a4ae442b25",
    "0b11": "cdf02fb9e017ed1addd0aeda56336cfa6b15d73e81ff826cf3c348a4ae442b25",
    "0b12": "cdf02fb9e017ed1addd0aeda56336cfa6b15d73e81ff826cf3c348a4ae442b25",
    "0b13": "8dd54f261a975e9e211501c3cc11838dcdf2807bf118a2b120507a7a091dc4e2",
    "0b16": "8dd54f261a975e9e211501c3cc11838dcdf2807bf118a2b120507a7a091dc4e2",
    "0b17": "019da3407c3f9cf25ef2cc1772271a2fdfbab801dba19ddb837f0612cc6767d3",
    "0b18": "b3f7ac1485fe6f1e52092bd128811f4365f324eaca5a23afd835a5371ddeb70e",
    "b18-0b0b": "75eca3b2b615d8083bfb31d346a6a35eda998cc53deff1163c0f389dc87523aa",
    "b18-0b0c": "75eca3b2b615d8083bfb31d346a6a35eda998cc53deff1163c0f389dc87523aa",
    "b18-0b0e": "75eca3b2b615d8083bfb31d346a6a35eda998cc53deff1163c0f389dc87523aa",
    "b18-0b10": "75eca3b2b615d8083bfb31d346a6a35eda998cc53deff1163c0f389dc87523aa",
    "b18-0b14": "75eca3b2b615d8083bfb31d346a6a35eda998cc53deff1163c0f389dc87523aa",
    "b18-0b18": "75eca3b2b615d8083bfb31d346a6a35eda998cc53deff1163c0f389dc87523aa",
    "b18-0b19": "75eca3b2b615d8083bfb31d346a6a35eda998cc53deff1163c0f389dc87523aa",
    "b18-0b1a": "1c61b580ca2d697d76a9c63fb17916c68b82ffc2a1c4d875afb7ebe728569f62",
    "b18-0b1c": "1c61b580ca2d697d76a9c63fb17916c68b82ffc2a1c4d875afb7ebe728569f62",
    "b18-0b1e": "1c61b580ca2d697d76a9c63fb17916c68b82ffc2a1c4d875afb7ebe728569f62",
    "b18-0b20": "906dfb08ac9fb99658fe8b9c63c2ab295623aa5061ace4fc426933efe9f2ccd6",
    "b18-0b22": "906dfb08ac9fb99658fe8b9c63c2ab295623aa5061ace4fc426933efe9f2ccd6",
    "0b1d": "906dfb08ac9fb99658fe8b9c63c2ab295623aa5061ace4fc426933efe9f2ccd6",
    "0b21": "906dfb08ac9fb99658fe8b9c63c2ab295623aa5061ace4fc426933efe9f2ccd6",
    "0b22": "b2c92850f9eafdb55745755146969752f1bad837c6a3fc8dae7b130f02cb2b3e",
    "0b23": "d73e84816b3ea50fa776f939821a2a671eb878fe92689c127b3b5a9cfbef27fe",
    "0b27": "d73e84816b3ea50fa776f939821a2a671eb878fe92689c127b3b5a9cfbef27fe",
    "0b28": "22ba5e872644a6b5be762bfd7647a2b2bf064089e566527582165bce6572edee",
    "0b29": "d2689954b0568fac034d5ed3fe66ad9aa020cf92270872f2e35392a58a259489",
    "0b29-callee": "5b41a551788f350d29747b56532dd1870f40df68b24f2174b26313e04865241a",
    "0b2e": "02f72986516eb643b98660fcd6eb13ea6060143d8691f4e798c7f46af99d2fe3",
    "0b2e-callee": "555fbb65e3b07a4cdcf8a0b9243d98aab15815b9cd97598363e0a9e130c7630d",
    "0b33": "55f1df5c2426711915daf6794e9c2a9cc74d2cc8c157e30ec8d147bdf9438c97",
    "0b37": "55f1df5c2426711915daf6794e9c2a9cc74d2cc8c157e30ec8d147bdf9438c97",
    "0b38": "55f1df5c2426711915daf6794e9c2a9cc74d2cc8c157e30ec8d147bdf9438c97",
    "0b39": "8233247812c05a02ff36e884c4686d448c25d90bb026ef4eaf45a9412abbddde",
    "0b3c": "8233247812c05a02ff36e884c4686d448c25d90bb026ef4eaf45a9412abbddde",
    "0b3d": "f4b98c8e5e6bf47d01fa646a99dc3f49fc4296ec4f285fc1e911024bea646e0e",
    "0b3d-callee": "9a4f0b271ed35080ff7e24a0dcce3be7e943d2d3f6294264b569097287fa97a2",
    "0b42": "2f625429daac31f026ed32904777f0a9c3e6d7b17aabb369b79983062feacc20",
    "0b46": "2f625429daac31f026ed32904777f0a9c3e6d7b17aabb369b79983062feacc20",
    "0b47": "2f625429daac31f026ed32904777f0a9c3e6d7b17aabb369b79983062feacc20",
    "0b48": "2f625429daac31f026ed32904777f0a9c3e6d7b17aabb369b79983062feacc20",
    "0b4b": "2f625429daac31f026ed32904777f0a9c3e6d7b17aabb369b79983062feacc20",
    "0b4c": "7946c0394b6f7cf743040a01b9e082e1655294d26d41ff9381acebd14d0c0aaa",
    "0b4d": "c2ff1c0acf23e50a4f845218c302f7e211c33daef242c00fb9e01f094cac7806",
    "0b4d-callee": "5c0e49391ed3c3ee5773cef1d601f476e13d7ed7fbb1ad3f076ffbbe9d53ff35",
    "0b52": "c7eaad2b345624933743f1018b11ce7c34ec419d3864d8a23fda624a03cdb246",
    "0b56": "c7eaad2b345624933743f1018b11ce7c34ec419d3864d8a23fda624a03cdb246",
    "0b57": "6f85589fbc3a48a20e767da67abd92cfc1b76411317acbb1d1659d0770c7ab01",
    "0b58": "dbd0110b3a3dabea050fe7e3dce5d0ab5b6514545909ea7f313cde48f0e84c38",
    "0b5b": "dbd0110b3a3dabea050fe7e3dce5d0ab5b6514545909ea7f313cde48f0e84c38",
    "0b5c": "d1c8f35237d7b8c960c759714c64f7745560dd87824a1e74023e11c920ff27a6",
    "0b5c-callee": "51609e79a3023deeb6bab18c84725185ab7677596f866d3c803ab4aa86da21b1",
    "0b61": "b7fc8446b04be6e233962389d66cfb15d13a7db88a5adece221bf1644ca8d302",
    "0b61-callee": "16de77c3b6f552a98eb7277833cf1e2453c86be4764ab9e4c0ab07e24f88b8a8",
    "0b66": "d7fcd8fc66f7b1aaa04bcfebfb19f43b69cf48e26ee2f9cddc63a5788aa09989",
    "0b69": "d7fcd8fc66f7b1aaa04bcfebfb19f43b69cf48e26ee2f9cddc63a5788aa09989",
    "0b6a": "39f226ba148e3b9185946e52d56214bb79de55c6e0a4ae3f55b480182303c813",
    "0b6b": "dcd9dd31e9490cf154ad47f11401ba7b9cafdac84adb8e25ec0d9e4937a7c155",
    "0b6b-callee": "1181b220be1584c009f838c18085b94f4c0f50c0aaa0629e1d0f47c4758c1a9b",
    "0b70": "d175731ccdef48091f13a70579b8d8e6285ddc0c4164f7e706b378980dd380e9",
    "0b74": "d175731ccdef48091f13a70579b8d8e6285ddc0c4164f7e706b378980dd380e9",
    "0b75": "026c67726fdc38e7d53d9137cc10ed4501980183b00b6ada4e811de04bc327a7",
    "0b76": "269b3979d041126a987042c4771aab5a58c4c98338ace3716c0a8679ecd1ebca",
    "0b79": "269b3979d041126a987042c4771aab5a58c4c98338ace3716c0a8679ecd1ebca",
    "0b7a": "b746f807a1d33fac5565918189eb2488645ede5154dfd6334797150142555c75",
    "0b7a-callee": "ddf423d1c8130c5d9ad5d3f58a95db2d9ec96794b482663b28f3edd4fe0ca3ff",
    "0b7f": "3502b1ea5c015aff729f79761fcdd238b3694fc338a4c15e93b7697cdc291c10",
    "0bcb": "3502b1ea5c015aff729f79761fcdd238b3694fc338a4c15e93b7697cdc291c10",
    "0bcf": "3502b1ea5c015aff729f79761fcdd238b3694fc338a4c15e93b7697cdc291c10",
    "0bd1": "3502b1ea5c015aff729f79761fcdd238b3694fc338a4c15e93b7697cdc291c10",
    "0bd4": "3502b1ea5c015aff729f79761fcdd238b3694fc338a4c15e93b7697cdc291c10",
    "0bd5": "e4b7b44d077c6803706672ea9dbf91b3652583276acd25e1db4fe4741f74f391",
    "0bd6": "3846c23f6c80b556c2b7318631624c448de246510393f8b77317a7a6debf0715",
    "0bda": "3846c23f6c80b556c2b7318631624c448de246510393f8b77317a7a6debf0715",
    "0bdb": "3846c23f6c80b556c2b7318631624c448de246510393f8b77317a7a6debf0715",
    "0bdc": "eca4c73d411f9b9deede7869973dc07bcfa5ff82fbd7455b45feadc1f5e16223",
    "0bdf": "eca4c73d411f9b9deede7869973dc07bcfa5ff82fbd7455b45feadc1f5e16223",
    "0be0": "9a8fea13fb9f2999fc4412b63fa0cb09bb376c6251d01fdd664e615d0a723081",
    "0be0-callee": "4742026c4a49dc69d2cf435758ed90b0dceb888f991ece0fe5ff4c440511fe19",
    "0be5": "c7f110ed291079b215af94253bfe08f1600be616d902a388fca6eefc0d327d49",
    "0bfb": "c7f110ed291079b215af94253bfe08f1600be616d902a388fca6eefc0d327d49",
    "0bff": "c7f110ed291079b215af94253bfe08f1600be616d902a388fca6eefc0d327d49",
    "0c00": "c17e4489e8dc09d3cb244fedd69516bb0ac09c3a6d15e1a43e059905a5629a36",
    "0c01": "5baf71b9c6b9880efc24b9783a5a2a6b18d0ba9678d442467d519f80f01a59e3",
    "0c05": "5baf71b9c6b9880efc24b9783a5a2a6b18d0ba9678d442467d519f80f01a59e3",
    "0c06": "5baf71b9c6b9880efc24b9783a5a2a6b18d0ba9678d442467d519f80f01a59e3",
    "0c07": "516587d673c30a6b658238f5ac59c970768c66d7c47eb5f5ed7b4ce57472d2df",
    "0c07-callee": "020858272dd6f42826aea0383dd9bfdd2d77cb308cab69f9f9ff67d3124ca5b0",
    "0c0c": "fe3f29feba18faa1b656cfd4a4a5642cfc55ba2d73b9ac92a2a94521f78bb855",
    "0c10": "fe3f29feba18faa1b656cfd4a4a5642cfc55ba2d73b9ac92a2a94521f78bb855",
    "0c11": "fe3f29feba18faa1b656cfd4a4a5642cfc55ba2d73b9ac92a2a94521f78bb855",
    "0c12": "235be2d3409efecc26590eba98dc1a909bbdd5463a06b9b43ae7b9dfdb7f99ed",
    "0c12-callee": "9d70045592593d967c90e4c8effbd1b155ad3703c81c812f3e90cf3d272948e8",
    "0c17": "dc6e06db48c186233aacdfc3b29dd566340afa44b2d12500dc82b85d05e5c1aa",
    "0c1b": "dc6e06db48c186233aacdfc3b29dd566340afa44b2d12500dc82b85d05e5c1aa",
    "0c1c": "dc6e06db48c186233aacdfc3b29dd566340afa44b2d12500dc82b85d05e5c1aa",
    "0c1d": "c7df80bc5ec7963f99bbc63bee5b0891bb617c316dad5dd9f0de7d032d690547",
    "0c1f": "c7df80bc5ec7963f99bbc63bee5b0891bb617c316dad5dd9f0de7d032d690547",
    "0c20": "e232461e7bbfe9657dc9e411882d5d4f86844ef50d42766c4885d1181865ad86",
    "0c24": "e232461e7bbfe9657dc9e411882d5d4f86844ef50d42766c4885d1181865ad86",
    "0c25": "2564a89490d5fb15c84f429a0229e62f0e87e80509296017a32b10c75392d010",
    "0c26": "517973b07ec36bacee930f131a45604a76f49079d6878d9f2c0a584477f7b686"
}

def word(image, off):
    return int.from_bytes(image[0x1D160+off:0x1D160+off+2], 'little')


def inputs(root):
    raw = (root / 'native.log').read_bytes()
    assert sha256(raw).hexdigest() == 'facac7976b2a533f95744d59b9ad59da4fcb0236b238bca4a0ee5b7cdd08badb'
    clean = re.sub(chr(27)+r'\[[0-?]*[ -/]*[@-~]', '', raw.decode(errors='replace'))
    rows = re.findall(r"LOG: EV of 'CS IP DS ES FS GS SS SP BP AX BX CX DX SI DI FLAGS' is:\s*LOG: ([0-9a-f ]+)", clean)
    assert len(rows) == len(RAM_HASHES) == 131
    cpus = {n: [int(v, 16) for v in row.split()] for n, row in zip(RAM_HASHES, rows)}
    images = {n: mapped.pins.checked(root / f'ram-{n}.bin', 655360, h) for n, h in RAM_HASHES.items()}
    game = mapped.pins.checked(root / 'POOLRAD/GAME.OVR', 206105, mapped.pins.GAME_HASH)
    start = mapped.pins.checked(root / 'POOLRAD/START.EXE', 65231, mapped.pins.START_HASH)
    mapped.pins.checked(root / 'POOLRAD/SAVGAMA.DAT', 13137, mapped.pins.SAVE_HASH)
    original = game[0x1EAD2:0x1EEAA]
    assert sha256(original).hexdigest() == '715424407210de3824c3a0218cc29033f04bb357ec718aa24a7995d9452d1e36'
    # Independently known loaded identity, including all 47 far relocations.
    loaded = images['19d7'][0x2A8E0+0xAEC:0x2A8E0+0xEC4]
    assert sha256(loaded).hexdigest() == '4da70731d0fe4afe0387c8df693aea3420b8c9c1916be920e79f16e04ad41ccc'
    differences = [i for i, (a, b) in enumerate(zip(original, loaded)) if a != b]
    assert len(differences) == 94
    for i in range(0, 94, 2):
        p, q = differences[i:i+2]
        assert q == p+1 and original[p-3] == 0x9A
        assert int.from_bytes(loaded[p:p+2], 'little') == int.from_bytes(original[p:p+2], 'little')+0x824
    calls = {0xB29: (0x1026, 0x425), 0xB2E: (0x131C, 0xB98), 0xB3D: (0x131C, 0xB25),
             0xB4D: (0x131C, 0xB0B), 0xB5C: (0x131C, 0xC3B), 0xB61: (0x131C, 0xB98),
             0xB6B: (0x131C, 0xB98), 0xB7A: (0x131C, 0xB25), 0xBE0: (0x131C, 0xB25),
             0xC07: (0x131C, 0xB0B), 0xC12: (0x131C, 0xB98)}
    mapped.code(root, images['19d7'])  # Existing CBC/B57/B98/B25 code identity.
    for n, values in cpus.items():
        assert len(values) == 16 and values == [json.loads((root / f'cpu-{n}.json').read_text())[f] for f in NAMES]
        if n.endswith('-callee'): expected = calls[int(n.split('-')[0], 16)]
        elif '-' in n: expected = (0x131C, int(n.split('-')[1], 16))
        else: expected = (0x2A8E, int(n, 16))
        assert values[:2] == list(expected), n
        image = images[n]
        assert image[0x2A8E0+0xAEC:0x2A8E0+0xEC4] == loaded
        assert image[0x2A8E0+0x19D7:0x2A8E0+0x19DA] == game[0x1DFE6+0x19D7:0x1DFE6+0x19DA]
        resident.original_code(root, image)  # Unchanged 0425/0356/string helpers.
        body = start[0xB781:0xB79B]
        assert sha256(body).hexdigest() == '4681f1935f55d71f05afab0b13fff0505aa17e9a04fb372ff559ef28ed88058f'
        assert image[0x131C0+0xB0B:0x131C0+0xB25] == body
    assert cpus['19d7'] == [0x2A8E,0x19D7,0x14A0,0x42AB,0,0,0x1D16,0x37AA,
                            0x3DCA,1,0x44AC,8,0x449A,0x449A,0x3BD4,0x7202]
    assert [word(images['0aec'], 0x37A8+2*i) for i in range(8)] == [0x19DA,0x2A8E,1,1,8,0x449A,0x3BD4,0x1D16]
    assert cpus['0af3'][7:9] == [0x33EC,0x37A6] and cpus['0af3'][-1] == 0x7212
    assert [word(images['0c26'], 0x33DE+2*i) for i in range(5)] == [0x3626,0x1D16,0,0x35EE,0x1D16]
    assert cpus['0c26'] == [0x2A8E,0xC26,0x14A0,0x1D16,0,0,0x1D16,0x33DE,
                            0x37A6,0,0x33E0,0,0x14A0,0x3624,0x3626,0x7202]
    assert sum(a != b for a, b in zip(images['19d7'], images['0c26'])) == 185
    return cpus, images


def differential(lib, uc, label, image, values, gate=(0xC26, 1), checkpoints=()):
    memory = ct.create_string_buffer(image, len(image))
    state = State(CPU(**dict(zip(NAMES, values))), ct.cast(memory, ct.POINTER(ct.c_uint8)), len(image))
    uc.mem_write(0, image)
    for reg, value in zip(mapped.REGS, values): uc.reg_write(reg, value)
    pending = list(checkpoints); seen = own = 0; started = time.monotonic()
    print(f'{label}: starting; limit 200000 instructions/{CASE_TIMEOUT:g}s', flush=True)
    for step in range(200000):
        pc = (state.cpu.cs, state.cpu.ip)
        if time.monotonic()-started >= CASE_TIMEOUT: raise TimeoutError(f'{label}: deadline at step {step}, PC {pc}')
        before_cpu, before_ram = cpu_values(state.cpu), memory.raw
        report = Report()
        status = lib.pr_slot_a_loader_0aec_step(ct.byref(state), ct.byref(report))
        if status:
            assert pc == (0x2A8E, gate[0]) and status == gate[1] and not pending, (label, step, pc, status)
            assert cpu_values(state.cpu) == before_cpu and memory.raw == before_ram
            if status == 1:
                assert (report.dependency_valid, report.caller, report.call_pc, report.callee_segment,
                        report.callee_pc, report.return_pc, report.caller_is_overlay) == (1,0x6F,0xC26,0x802,0x25D,0xC2B,1)
            else: assert not report.dependency_valid
            print(f'{label}: {step} full-state comparisons ({own} caller/loader, {step-own} unchanged helpers); '
                  f'{seen} native checkpoints; blocked at {gate[0]:04X}; elapsed={time.monotonic()-started:.1f}s', flush=True)
            return step, own, seen
        assert not report.dependency_valid
        # REP stays one C step; retain a deadline while the oracle repeats it.
        for repetition in range(65537):
            if repetition % 256 == 0 and time.monotonic()-started >= CASE_TIMEOUT:
                raise TimeoutError(f'{label}: REP deadline at {pc}')
            uc.emu_start((uc.reg_read(mapped.REGS[0]) << 4)+uc.reg_read(mapped.REGS[1]), 0x100000, count=1)
            if (uc.reg_read(mapped.REGS[0]), uc.reg_read(mapped.REGS[1])) != pc: break
        else: raise AssertionError(f'{label}: original instruction did not finish')
        compare_cpu(cpu_values(state.cpu), mapped.values(uc), f'{label} after {pc}')
        assert memory.raw == bytes(uc.mem_read(0, len(image))), (label, step, pc, 'RAM')
        own += pc[0] == 0x2A8E
        if pending and cpu_values(state.cpu)[:2] == pending[0][1][:2]:
            name, expected_cpu, expected_ram = pending.pop(0)
            compare_cpu(cpu_values(state.cpu), expected_cpu, name)
            assert memory.raw == expected_ram, name
            seen += 1
        if step and step % 1000 == 0:
            print(f'{label}: step {step}, PC {state.cpu.cs:04X}:{state.cpu.ip:04X}; '
                  f'native={seen}/{seen+len(pending)}; elapsed={time.monotonic()-started:.1f}s', flush=True)
    raise AssertionError(f'{label}: instruction bound')


def one_step(lib, uc, image, values):
    memory = ct.create_string_buffer(image, len(image))
    state = State(CPU(**dict(zip(NAMES, values))), ct.cast(memory, ct.POINTER(ct.c_uint8)), len(image))
    uc.mem_write(0, image)
    for reg, value in zip(mapped.REGS, values): uc.reg_write(reg, value)
    report = Report()
    assert lib.pr_slot_a_loader_0aec_step(ct.byref(state), ct.byref(report)) == 0
    assert not report.dependency_valid
    mapped.one(uc)
    compare_cpu(cpu_values(state.cpu), mapped.values(uc), 'independent loader instruction')
    assert memory.raw == bytes(uc.mem_read(0, len(image)))
    return memory.raw, cpu_values(state.cpu)


def instruction_cases(lib, uc, cpus, images):
    total = 0
    for sp in (0,0x8000,0x3BA,0xFFFF):
        values = cpus['0aef'].copy(); values[7] = sp
        one_step(lib, uc, images['0aef'], values); total += 1
    for value in (0,1,0x7F,0x80,0xFF):
        image = bytearray(images['0b07']); image[0x2090C] = value
        after, values = one_step(lib, uc, bytes(image), cpus['0b07'])
        one_step(lib, uc, after, values); total += 2
    for name in ('0b02','0b18','0b29','0b2e','0b3d','0b4d','0b5c','0b61','0b6b','0b7a','0be0','0c07','0c12'):
        values = cpus[name].copy(); values[7] = 0
        one_step(lib, uc, images[name], values); total += 1
    for name in ('0b58','0c1d'):
        values = cpus[name].copy(); values[9] = 0xA5FF
        one_step(lib, uc, images[name], values); total += 1
    print(f'Independent owned instructions: {total} full CPU/RAM original-byte comparisons; '
          'SUB flags, CMP/branch flags, all thirteen wrapped far frames and AH preservation', flush=True)
    return total


def main():
    if len(sys.argv) != 3: raise SystemExit('usage: audit_slot_a_loader_0aec_prefix.py LIBRARY NATIVE_LOADER_ROOT')
    if unicorn.__version__ != '2.1.4' or not __debug__: raise SystemExit('requires Unicorn 2.1.4 and non-optimized assertions')
    library, root = (Path(p).resolve(strict=True) for p in sys.argv[1:])
    if any(not p.is_relative_to((ROOT / 'resources').resolve()) for p in (library, root)):
        raise SystemExit('library and captures must stay in this clone resources/')
    cpus, images = inputs(root)
    lib = ct.CDLL(str(library))
    lib.pr_slot_a_loader_0aec_step.argtypes = [ct.POINTER(State), ct.POINTER(Report)]
    lib.pr_slot_a_loader_0aec_step.restype = ct.c_int
    uc = mapped.emulator(images['19d7'], cpus['19d7'])
    total = own = native = 0
    def run(*args, **kwargs):
        nonlocal total, own, native
        n, o, c = differential(lib, uc, *args, **kwargs)
        total += n; own += o; native += c
    run('natural loader prefix', images['19d7'], cpus['19d7'],
        checkpoints=[(n, cpus[n], images[n]) for n in list(RAM_HASHES)[1:]])
    # Each synthetic case starts independently; none replaces an in-flight
    # state or supplies an allocator result to a continuous replay.
    values = cpus['19d7'].copy(); values[-1] = 0x7ED7; values[9] = 0xABCD
    image = bytearray(images['19d7']); image[0x2090C] = 0
    run('synthetic entry AH/DF/stale flags to zero-path gate', bytes(image), values, gate=(0xB81,2))
    image = bytearray(images['19d7']); values = cpus['19d7'].copy(); values[7] = 0
    image[0x1D160:0x1D16E] = b''.join(v.to_bytes(2, 'little') for v in (0x2A8E,0,1,8,0x449A,0x3BD4,0x1D16))
    run('synthetic wrapped near frame and locals', bytes(image), values, gate=(0xB81,2))
    for length in (0,1,255):
        image = bytearray(images['19d7'])
        payload = bytes([length])+bytes(0x61+i%26 for i in range(length))
        image[0x23160:0x23160+len(payload)] = payload
        image[0x20914:0x20918] = b''.join(v.to_bytes(2, 'little') for v in (0x6000,0x1D16))
        image[0x2090C] = 0
        run(f'synthetic own basename length {length} to zero-path gate', bytes(image), cpus['19d7'], gate=(0xB81,2))
    image = bytearray(images['0bcb']); image[0x2090E] = 0
    run('synthetic independent second-flag zero boundary', bytes(image), cpus['0bcb'], gate=(0xBE7,2))
    targeted = instruction_cases(lib, uc, cpus, images)
    total += targeted; own += targeted
    print(f'Loader prefix audit passed: {total} full-state C/original-byte instruction comparisons '
          f'({own} new caller/loader, {total-own} unchanged helper steps); {native} distinct post-input native checkpoints; '
          '131 untouched native rows; file-open, loader remainder and zero-flag paths stay blocked', flush=True)


if __name__ == '__main__': main()
